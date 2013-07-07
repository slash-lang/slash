#include <slash/lex.h>
#include <slash/ast.h>
#include <slash/parse.h>
#include <slash/eval.h>
#include <slash/lib/float.h>
#include <slash/lib/number.h>
#include <slash/lib/regexp.h>
#include <slash/string.h>
#include <slash/object.h>
#include <slash/method.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static sl_token_t*
token(sl_parse_state_t* ps)
{
    return &ps->tokens[ps->current_token - 1];
}

static sl_token_t*
peek_token(sl_parse_state_t* ps)
{
    return &ps->tokens[ps->current_token];
}

static sl_token_t*
peek_token_n(sl_parse_state_t* ps, int n)
{
    return &ps->tokens[ps->current_token + n - 1];
}

static sl_token_t*
next_token(sl_parse_state_t* ps)
{
    sl_token_t* tok = &ps->tokens[ps->current_token++];
    ps->line = tok->line;
    return tok;
}

static void
error(sl_parse_state_t* ps, SLVAL err, sl_token_t* tok)
{
    err = sl_make_error2(ps->vm, ps->vm->lib.SyntaxError, err);
    sl_error_add_frame(ps->vm, err, sl_make_cstring(ps->vm, "<parser>"), sl_make_cstring(ps->vm, (char*)ps->filename), sl_make_int(ps->vm, tok->line));
    sl_throw(ps->vm, err);
}

static void
unexpected(sl_parse_state_t* ps, sl_token_t* tok)
{
    SLVAL err;
    if(tok->type != SL_TOK_END) {
        err = sl_make_formatted_string(ps->vm, "Unexpected %QV", tok->str);
    } else {
        err = sl_make_cstring(ps->vm, "Unexpected end of file");
    }
    error(ps, err, tok);
}

static sl_node_base_t*
expression(sl_parse_state_t* ps);

static sl_node_base_t*
primary_expression(sl_parse_state_t* ps);

static sl_node_base_t*
statement(sl_parse_state_t* ps);

static sl_node_base_t*
send_expression(sl_parse_state_t* ps, sl_node_base_t* recv);

static sl_node_base_t*
call_expression(sl_parse_state_t* ps);

static sl_node_base_t*
low_precedence_logical_expression(sl_parse_state_t* ps);

static SLID
def_expression_method_name(sl_parse_state_t* ps);

static sl_token_t*
expect_token(sl_parse_state_t* ps, sl_token_type_t type)
{
    sl_token_t* tok = next_token(ps);
    if(tok->type != type) {
        unexpected(ps, tok);
    }
    return tok;
}

static sl_node_base_t*
body_expression(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node(ps);
    sl_node_base_t* node;
    expect_token(ps, SL_TOK_OPEN_BRACE);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        node = statement(ps);
        if(node) {
            sl_seq_node_append(ps, seq, node);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_BRACE);
    return (sl_node_base_t*)seq;
}

static sl_node_base_t*
if_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *condition, *if_true, *if_false = NULL;
    int negate_condition = 0;
    if(peek_token(ps)->type == SL_TOK_ELSIF) {
        expect_token(ps, SL_TOK_ELSIF);
    } else if(peek_token(ps)->type == SL_TOK_UNLESS) {
        expect_token(ps, SL_TOK_UNLESS);
        negate_condition = 1;
    } else {
        expect_token(ps, SL_TOK_IF);
    }
    condition = expression(ps);
    if(negate_condition) {
        condition = sl_make_unary_node(ps, condition, SL_NODE_NOT);
    }
    if_true = body_expression(ps);
    if(peek_token(ps)->type == SL_TOK_ELSIF) {
        if_false = if_expression(ps);
    }
    if(peek_token(ps)->type == SL_TOK_ELSE) {
        next_token(ps);
        if_false = body_expression(ps);
    }
    return sl_make_if_node(ps, condition, if_true, if_false);
}

static sl_node_base_t*
switch_expression(sl_parse_state_t* ps)
{
    expect_token(ps, SL_TOK_SWITCH);
    sl_node_base_t* value = expression(ps);
    expect_token(ps, SL_TOK_OPEN_BRACE);
    size_t case_count = 0, case_cap = 2;
    sl_node_switch_case_t* cases = sl_alloc(ps->vm->arena, sizeof(sl_node_switch_case_t) * case_cap);
    sl_node_base_t* else_body = NULL;
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        if(peek_token(ps)->type == SL_TOK_ELSE) {
            next_token(ps);
            else_body = body_expression(ps);
            break;
        }
        if(case_count + 1 >= case_cap) {
            case_cap *= 2;
            cases = sl_realloc(ps->vm->arena, cases, sizeof(sl_node_switch_case_t) * case_cap);
        }
        cases[case_count].value = expression(ps);
        cases[case_count].body = body_expression(ps);
        case_count++;
    }
    expect_token(ps, SL_TOK_CLOSE_BRACE);
    return sl_make_switch_node(ps, value, case_count, cases, else_body);
}

static sl_node_base_t*
while_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *condition, *body;
    sl_parse_scope_t scope;
    int until = 0;
    if(peek_token(ps)->type == SL_TOK_UNTIL) {
        next_token(ps);
        until = 1;
    } else {
        expect_token(ps, SL_TOK_WHILE);
    }
    condition = expression(ps);
    if(until) {
        condition = sl_make_unary_node(ps, condition, SL_NODE_NOT);
    }
    scope.prev = ps->scope;
    scope.flags = scope.prev->flags | SL_PF_CAN_NEXT_LAST;
    ps->scope = &scope;
    body = body_expression(ps);
    ps->scope = scope.prev;
    if(scope.flags & SL_PF_SCOPE_CLOSURE) {
        ps->scope->flags |= SL_PF_SCOPE_CLOSURE;
    }
    return sl_make_while_node(ps, condition, body);
}

static sl_node_base_t*
for_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *lval, *expr, *body, *else_body = NULL;
    sl_node_seq_t* seq_lval;
    sl_token_t* tok;
    sl_parse_scope_t scope;
    expect_token(ps, SL_TOK_FOR);
    /* save current token to allow rewinding and erroring */
    tok = peek_token(ps);
    lval = call_expression(ps);
    if(!sl_node_is_lval(lval)) {
        unexpected(ps, tok);
    }
    if(peek_token(ps)->type == SL_TOK_COMMA || peek_token(ps)->type == SL_TOK_FAT_COMMA) {
        seq_lval = sl_make_seq_node(ps);
        seq_lval->nodes[seq_lval->node_count++] = lval;
        while(peek_token(ps)->type == SL_TOK_COMMA || peek_token(ps)->type == SL_TOK_FAT_COMMA) {
            next_token(ps);
            if(seq_lval->node_count == seq_lval->node_capacity) {
                seq_lval->node_capacity *= 2;
                seq_lval->nodes = sl_realloc(ps->vm->arena, seq_lval->nodes, sizeof(sl_node_base_t*) * seq_lval->node_capacity);
            }
            tok = peek_token(ps);
            lval = call_expression(ps);
            if(!sl_node_is_lval(lval)) {
                unexpected(ps, tok);
            }
            seq_lval->nodes[seq_lval->node_count++] = lval;
        }
        lval = sl_make_array_node(ps, seq_lval->node_count, seq_lval->nodes);
    }
    expect_token(ps, SL_TOK_IN);
    expr = expression(ps);
    
    scope.prev = ps->scope;
    scope.flags = scope.prev->flags | SL_PF_CAN_NEXT_LAST;
    ps->scope = &scope;
    body = body_expression(ps);
    ps->scope = scope.prev;
    if(scope.flags & SL_PF_SCOPE_CLOSURE) {
        ps->scope->flags |= SL_PF_SCOPE_CLOSURE;
    }
    
    if(peek_token(ps)->type == SL_TOK_ELSE) {
        next_token(ps);
        else_body = body_expression(ps);
    }
    return sl_make_for_node(ps, lval, expr, body, else_body);
}

static sl_node_base_t*
class_expression(sl_parse_state_t* ps)
{
    expect_token(ps, SL_TOK_CLASS);
    sl_token_t* tok = expect_token(ps, SL_TOK_CONSTANT);
    SLID name = sl_intern2(ps->vm, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
    sl_node_base_t *extends, *body;
    if(peek_token(ps)->type == SL_TOK_EXTENDS) {
        next_token(ps);
        extends = expression(ps);
    } else {
        extends = NULL;
    }
    body = body_expression(ps);
    return sl_make_class_node(ps, name, extends, body);
}

static SLID
def_expression_method_name(sl_parse_state_t* ps)
{
    switch(peek_token(ps)->type) {
        case SL_TOK_IDENTIFIER:
            return sl_intern2(ps->vm, next_token(ps)->str);
        /* operators: */
        case SL_TOK_SHIFT_LEFT:
        case SL_TOK_SHIFT_RIGHT:
        case SL_TOK_DBL_EQUALS:
        case SL_TOK_NOT_EQUALS:
        case SL_TOK_SPACESHIP:
        case SL_TOK_LTE:
        case SL_TOK_LT:
        case SL_TOK_GTE:
        case SL_TOK_GT:
        case SL_TOK_PLUS:
        case SL_TOK_POW:
        case SL_TOK_TIMES:
        case SL_TOK_DIVIDE:
        case SL_TOK_MOD:
        case SL_TOK_CARET:
        case SL_TOK_AMP:
        case SL_TOK_PIPE:
            return sl_intern2(ps->vm, next_token(ps)->str);
        /* operators that can also be unary: */
        case SL_TOK_MINUS:
        case SL_TOK_TILDE:
        {
            sl_token_t* tok = next_token(ps);
            if(peek_token(ps)->type == SL_TOK_SELF) {
                return sl_intern2(ps->vm, sl_string_concat(ps->vm, tok->str, next_token(ps)->str));
            } else {
                return sl_intern2(ps->vm, tok->str);
            }
            break;
        }
        /* keywords: */
        case SL_TOK_LAST:
        case SL_TOK_NEXT:
            return sl_intern2(ps->vm, next_token(ps)->str);
        case SL_TOK_OPEN_BRACKET:
            if(peek_token_n(ps, 2)->type == SL_TOK_CLOSE_BRACKET) {
                next_token(ps);
                next_token(ps);
                return sl_intern(ps->vm, "[]");
            }
        default:
            unexpected(ps, next_token(ps));
            SLID dummy;
            return dummy; /* never reached */
    }
}

static sl_node_base_t*
def_expression(sl_parse_state_t* ps)
{
    SLID name;
    sl_node_base_t* on = NULL;
    sl_node_base_t* body;
    sl_token_t* tok;
    size_t req_arg_count = 0, req_arg_cap = 2;
    sl_string_t** req_args = sl_alloc(ps->vm->arena, sizeof(sl_string_t*) * req_arg_cap);
    size_t opt_arg_count = 0, opt_arg_cap = 2;
    sl_node_opt_arg_t* opt_args = sl_alloc(ps->vm->arena, sizeof(sl_node_opt_arg_t) * opt_arg_cap);
    sl_parse_scope_t scope;
    sl_string_t* rest_arg = NULL;
    expect_token(ps, SL_TOK_DEF);
    switch(peek_token(ps)->type) {
        case SL_TOK_IDENTIFIER:
            if(peek_token_n(ps, 2)->type == SL_TOK_DOT) {
                on = sl_make_var_node(ps, SL_NODE_VAR, next_token(ps)->str);
                next_token(ps);
                name = def_expression_method_name(ps);
            } else {
                on = NULL;
                name = def_expression_method_name(ps);
            }
            break;
        case SL_TOK_SELF:
        case SL_TOK_IVAR:
        case SL_TOK_CVAR:
        case SL_TOK_CONSTANT:
            on = primary_expression(ps);
            expect_token(ps, SL_TOK_DOT);
            name = def_expression_method_name(ps);
            break;
        default:
            name = def_expression_method_name(ps);
    }
    if(peek_token(ps)->type == SL_TOK_EQUALS) {
        next_token(ps);
        name = sl_id_make_setter(ps->vm, name);
    }
    int at_opt_args = 0;
    if(peek_token(ps)->type != SL_TOK_OPEN_BRACE) {
        expect_token(ps, SL_TOK_OPEN_PAREN);
        if(peek_token(ps)->type == SL_TOK_SELF) {
            error(ps, sl_make_cstring(ps->vm, "not a chance"), peek_token(ps));
        }
        while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            tok = expect_token(ps, SL_TOK_IDENTIFIER);
            if(peek_token(ps)->type == SL_TOK_EQUALS) {
                at_opt_args = 1;
            }
            if(peek_token(ps)->type == SL_TOK_RANGE_EX) {
                rest_arg = (sl_string_t*)sl_get_ptr(
                    sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
                expect_token(ps, SL_TOK_RANGE_EX);
                break;
            } else if(at_opt_args) {
                expect_token(ps, SL_TOK_EQUALS);
                if(opt_arg_count >= opt_arg_cap) {
                    opt_arg_cap *= 2;
                    opt_args = sl_realloc(ps->vm->arena, opt_args, sizeof(sl_node_opt_arg_t) * opt_arg_cap);
                }
                opt_args[opt_arg_count].name = (sl_string_t*)sl_get_ptr(
                    sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
                opt_args[opt_arg_count++].default_value = expression(ps);
            } else {
                if(req_arg_count >= req_arg_cap) {
                    req_arg_cap *= 2;
                    req_args = sl_realloc(ps->vm->arena, req_args, sizeof(sl_string_t*) * req_arg_cap);
                }
                req_args[req_arg_count++] = (sl_string_t*)sl_get_ptr(
                    sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            }
            if(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
                expect_token(ps, SL_TOK_COMMA);
            }
        }
        expect_token(ps, SL_TOK_CLOSE_PAREN);
    }
    scope.prev = ps->scope;
    scope.flags = SL_PF_CAN_RETURN;
    ps->scope = &scope;
    body = body_expression(ps);
    ps->scope = scope.prev;
    ps->scope->flags |= SL_PF_SCOPE_CLOSURE;
    return sl_make_def_node(ps, name, on, req_arg_count, req_args, opt_arg_count, opt_args, body, rest_arg);
}

static sl_node_base_t*
lambda_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* body;
    sl_token_t* tok;
    size_t arg_count = 0, arg_cap = 2;
    sl_string_t** args = sl_alloc(ps->vm->arena, sizeof(sl_string_t*) * arg_cap);
    sl_parse_scope_t scope;
    expect_token(ps, SL_TOK_LAMBDA);
    if(peek_token(ps)->type == SL_TOK_IDENTIFIER) {
        tok = next_token(ps);
        args[arg_count++] = (sl_string_t*)sl_get_ptr(
            sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
    } else if(peek_token(ps)->type != SL_TOK_OPEN_BRACE && peek_token(ps)->type != SL_TOK_DOT) {
        expect_token(ps, SL_TOK_OPEN_PAREN);
        while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            if(arg_count >= arg_cap) {
                arg_cap *= 2;
                args = sl_realloc(ps->vm->arena, args, sizeof(sl_string_t*) * arg_cap);
            }
            tok = expect_token(ps, SL_TOK_IDENTIFIER);
            args[arg_count++] = (sl_string_t*)sl_get_ptr(
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            if(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
                expect_token(ps, SL_TOK_COMMA);
            }
        }
        expect_token(ps, SL_TOK_CLOSE_PAREN);
    }
    scope.prev = ps->scope;
    scope.flags = SL_PF_CAN_RETURN;
    ps->scope = &scope;
    if(peek_token(ps)->type == SL_TOK_DOT) {
        next_token(ps);
        body = expression(ps);
    } else {
        body = body_expression(ps);
    }
    ps->scope = scope.prev;
    ps->scope->flags |= SL_PF_SCOPE_CLOSURE;
    return sl_make_lambda_node(ps, arg_count, args, body);
}

static sl_node_base_t*
try_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *body, *lval = NULL, *catch_body = NULL;
    sl_token_t* tok;
    expect_token(ps, SL_TOK_TRY);
    body = body_expression(ps);
    expect_token(ps, SL_TOK_CATCH);
    tok = peek_token(ps);
    if(tok->type != SL_TOK_OPEN_BRACE) {
        lval = primary_expression(ps);
        if(!sl_node_is_lval(lval)) {
            unexpected(ps, tok);
        }
    }
    catch_body = body_expression(ps);
    return sl_make_try_node(ps, body, lval, catch_body);
}

static sl_node_base_t*
send_with_args_expression(sl_parse_state_t* ps, sl_node_base_t* recv, SLID id)
{
    size_t argc = 0, cap = 2;
    sl_node_base_t** argv = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * cap);
    expect_token(ps, SL_TOK_OPEN_PAREN);
    while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
        if(argc >= cap) {
            cap *= 2;
            argv = sl_realloc(ps->vm->arena, argv, sizeof(sl_node_base_t*) * cap);
        }
        argv[argc++] = expression(ps);
        if(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            expect_token(ps, SL_TOK_COMMA);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_PAREN);
    return sl_make_send_node(ps, recv, id, argc, argv);
}

static sl_node_base_t*
array_expression(sl_parse_state_t* ps)
{
    size_t count = 0, cap = 2;
    sl_node_base_t** nodes = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * cap);
    expect_token(ps, SL_TOK_OPEN_BRACKET);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
        if(count >= cap) {
            cap *= 2;
            nodes = sl_realloc(ps->vm->arena, nodes, sizeof(sl_node_base_t*) * cap);
        }
        nodes[count++] = expression(ps);
        if(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
            expect_token(ps, SL_TOK_COMMA);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_BRACKET);
    return sl_make_array_node(ps, count, nodes);
}

static sl_node_base_t*
dict_expression(sl_parse_state_t* ps)
{
    size_t count = 0, cap = 2;
    sl_node_base_t** keys = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * cap);
    sl_node_base_t** vals = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * cap);
    expect_token(ps, SL_TOK_OPEN_BRACE);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        if(count >= cap) {
            cap *= 2;
            keys = sl_realloc(ps->vm->arena, keys, sizeof(sl_node_base_t*) * cap);
            vals = sl_realloc(ps->vm->arena, vals, sizeof(sl_node_base_t*) * cap);
        }
        keys[count] = expression(ps);
        expect_token(ps, SL_TOK_FAT_COMMA);
        vals[count] = expression(ps);
        count++;
        if(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
            expect_token(ps, SL_TOK_COMMA);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_BRACE);
    return sl_make_dict_node(ps, count, keys, vals);
}

static sl_node_base_t*
bracketed_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* node;
    expect_token(ps, SL_TOK_OPEN_PAREN);
    node = expression(ps);
    expect_token(ps, SL_TOK_CLOSE_PAREN);
    return node;
}

static sl_node_base_t*
regexp_expression(sl_parse_state_t* ps)
{
    sl_token_t* re = expect_token(ps, SL_TOK_REGEXP);
    sl_token_t* opts = expect_token(ps, SL_TOK_REGEXP_OPTS);
    return sl_make_immediate_node(ps, sl_make_regexp(ps->vm,
        re->as.str.buff, re->as.str.len,
        opts->as.str.buff, opts->as.str.len));
}

static sl_node_base_t*
static_string_expression(sl_parse_state_t* ps)
{
    sl_token_t* tok = expect_token(ps, SL_TOK_STRING);
    return sl_make_immediate_node(ps, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
}

static sl_node_base_t*
string_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* node = static_string_expression(ps);
    if(peek_token(ps)->type != SL_TOK_STRING_BEGIN_INTERPOLATION) {
        return node;
    }
    size_t comp_count = 1;
    size_t comp_cap = 4;
    sl_node_base_t** components = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * comp_cap);
    components[0] = node;
    while(peek_token(ps)->type == SL_TOK_STRING_BEGIN_INTERPOLATION) {
        if(comp_count + 2 >= comp_cap) {
            comp_cap *= 2;
            components = sl_realloc(ps->vm->arena, components, sizeof(sl_node_base_t*) * comp_cap);
        }
        next_token(ps);
        components[comp_count++] = expression(ps);
        expect_token(ps, SL_TOK_STRING_END_INTERPOLATION);
        components[comp_count++] = static_string_expression(ps);
    }
    return sl_make_interp_string_node(ps, components, comp_count);
}

static sl_node_base_t*
primary_expression(sl_parse_state_t* ps)
{
    sl_token_t* tok;
    sl_node_base_t* node;
    switch(peek_token(ps)->type) {
        case SL_TOK_INTEGER:
            tok = next_token(ps);
            return sl_make_immediate_node(ps, sl_integer_parse(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_FLOAT:
            return sl_make_immediate_node(ps, sl_make_float(ps->vm, next_token(ps)->as.dbl));
        case SL_TOK_STRING:
            return string_expression(ps);
        case SL_TOK_REGEXP:
            return regexp_expression(ps);
        case SL_TOK_CONSTANT:
            tok = next_token(ps);
            return sl_make_const_node(ps, NULL, sl_intern2(ps->vm, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len)));
        case SL_TOK_IDENTIFIER:
            tok = next_token(ps);
            return sl_make_var_node(ps, SL_NODE_VAR,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_TRUE:
            next_token(ps);
            return sl_make_immediate_node(ps, ps->vm->lib._true);
        case SL_TOK_FALSE:
            next_token(ps);
            return sl_make_immediate_node(ps, ps->vm->lib._false);
        case SL_TOK_NIL:
            next_token(ps);
            return sl_make_immediate_node(ps, ps->vm->lib.nil);
        case SL_TOK_SELF:
            next_token(ps);
            return sl_make_self_node(ps);
        case SL_TOK_IVAR:
            tok = next_token(ps);
            node = sl_make_var_node(ps, SL_NODE_IVAR,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            return node;
        case SL_TOK_CVAR:
            tok = next_token(ps);
            node = sl_make_var_node(ps, SL_NODE_CVAR,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            return node;
        case SL_TOK_IF:
        case SL_TOK_UNLESS:
            return if_expression(ps);
        case SL_TOK_SWITCH:
            return switch_expression(ps);
        case SL_TOK_WHILE:
        case SL_TOK_UNTIL:
            return while_expression(ps);
        case SL_TOK_FOR:
            return for_expression(ps);
        case SL_TOK_CLASS:
            return class_expression(ps);
        case SL_TOK_DEF:
            return def_expression(ps);
        case SL_TOK_LAMBDA:
            return lambda_expression(ps);
        case SL_TOK_TRY:
            return try_expression(ps);
        case SL_TOK_OPEN_BRACKET:
            return array_expression(ps);
        case SL_TOK_OPEN_PAREN:
            return bracketed_expression(ps);
        case SL_TOK_OPEN_BRACE:
            return dict_expression(ps);
        case SL_TOK_NEXT:
            tok = next_token(ps);
            if(!(ps->scope->flags & SL_PF_CAN_NEXT_LAST)) {
                error(ps, sl_make_cstring(ps->vm, "next invalid outside loop"), tok);
            }
            return sl_make_singleton_node(ps, SL_NODE_NEXT);
        case SL_TOK_LAST:
            tok = next_token(ps);
            if(!(ps->scope->flags & SL_PF_CAN_NEXT_LAST)) {
                error(ps, sl_make_cstring(ps->vm, "last invalid outside loop"), tok);
            }
            return sl_make_singleton_node(ps, SL_NODE_LAST);
        case SL_TOK_SPECIAL_FILE:
            next_token(ps);
            return sl_make_immediate_node(ps,
                sl_make_cstring(ps->vm, (char*)ps->filename));
        case SL_TOK_SPECIAL_LINE:
            return sl_make_immediate_node(ps,
                sl_make_int(ps->vm, next_token(ps)->line));
        case SL_TOK_RANGE_EX:
            next_token(ps);
            return sl_make_singleton_node(ps, SL_NODE_YADA_YADA);
        default:
            unexpected(ps, peek_token(ps));
            return NULL;
    }
}

static sl_node_base_t*
send_expression(sl_parse_state_t* ps, sl_node_base_t* recv)
{
    SLVAL id;
    sl_token_t* tok;
    tok = expect_token(ps, SL_TOK_IDENTIFIER);
    id = sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len);
    if(peek_token(ps)->type != SL_TOK_OPEN_PAREN) {
        return sl_make_send_node(ps, recv, sl_intern2(ps->vm, id), 0, NULL);
    }
    return send_with_args_expression(ps, recv, sl_intern2(ps->vm, id));
}

static sl_node_base_t*
call_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = primary_expression(ps);
    sl_node_base_t** nodes;
    size_t node_len;
    size_t node_cap;
    sl_token_t* tok;
    if(left->type == SL_NODE_VAR && peek_token(ps)->type == SL_TOK_OPEN_PAREN) {
        left = send_with_args_expression(ps, sl_make_self_node(ps),
            sl_intern2(ps->vm, sl_make_ptr((sl_object_t*)((sl_node_var_t*)left)->name)));
    }
    while(1) {
        tok = peek_token(ps);
        switch(tok->type) {
            case SL_TOK_DOT:
                next_token(ps);
                left = send_expression(ps, left);
                break;
            case SL_TOK_COLON:
                next_token(ps);
                left = sl_make_bind_method_node(ps, left, def_expression_method_name(ps));
                break;
            case SL_TOK_PAAMAYIM_NEKUDOTAYIM:
                next_token(ps);
                tok = expect_token(ps, SL_TOK_CONSTANT);
                left = sl_make_const_node(ps, left,
                    sl_intern2(ps->vm,
                        sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len)));
                break;
            case SL_TOK_OPEN_BRACKET:
                next_token(ps);
                node_cap = 1;
                node_len = 0;
                nodes = sl_alloc(ps->vm->arena, sizeof(SLVAL) * node_cap);
                while(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
                    if(node_len >= node_cap) {
                        node_cap *= 2;
                        nodes = sl_realloc(ps->vm->arena, nodes, sizeof(SLVAL) * node_cap);
                    }
                    nodes[node_len++] = expression(ps);
                    if(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
                        expect_token(ps, SL_TOK_COMMA);
                    }
                }
                expect_token(ps, SL_TOK_CLOSE_BRACKET);
                left = sl_make_send_node(ps, left, sl_intern(ps->vm, "[]"), node_len, nodes);
                break;
            default:
                return left;
        }
    }
}

static sl_node_base_t*
inc_dec_expression(sl_parse_state_t* ps)
{
    if(peek_token(ps)->type == SL_TOK_INCREMENT) {
        next_token(ps);
        return sl_make_prefix_mutate_node(ps, call_expression(ps), "succ");
    }
    if(peek_token(ps)->type == SL_TOK_DECREMENT) {
        next_token(ps);
        return sl_make_prefix_mutate_node(ps, call_expression(ps), "pred");
    }
    sl_node_base_t* lval = call_expression(ps);
    if(sl_node_is_mutable_lval(lval)) {
        if(peek_token(ps)->type == SL_TOK_INCREMENT) {
            next_token(ps);
            return sl_make_postfix_mutate_node(ps, lval, "succ");
        }
        if(peek_token(ps)->type == SL_TOK_DECREMENT) {
            next_token(ps);
            return sl_make_postfix_mutate_node(ps, lval, "pred");
        }
    }
    return lval;
}

static sl_node_base_t*
power_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = inc_dec_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    if(peek_token(ps)->type == SL_TOK_POW) {
        tok = next_token(ps);
        right = power_expression(ps);
        left = sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
    }
    return left;
}

static sl_node_base_t*
use_expression(sl_parse_state_t* ps)
{
    expect_token(ps, SL_TOK_USE);
    sl_node_base_t* node = call_expression(ps);
    if(node->type != SL_NODE_CONST) {
        error(ps, sl_make_cstring(ps->vm, "Expected constant reference in use expression"), token(ps));
    }
    for(sl_node_base_t* i = node; i && i->type == SL_NODE_CONST; i = ((sl_node_const_t*)i)->obj) {
        i->type = SL_NODE_USE;
    }
    return node;
}

static sl_node_base_t*
unary_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    sl_token_t* tok;
    switch(peek_token(ps)->type) {
        case SL_TOK_MINUS:
            tok = next_token(ps);
            expr = unary_expression(ps);
            return sl_make_send_node(ps, expr, sl_intern(ps->vm, "-self"), 0, NULL);
        case SL_TOK_TILDE:
            tok = next_token(ps);
            expr = unary_expression(ps);
            return sl_make_send_node(ps, expr, sl_intern(ps->vm, "~self"), 0, NULL);
        case SL_TOK_NOT:
            next_token(ps);
            expr = unary_expression(ps);
            return sl_make_unary_node(ps, expr, SL_NODE_NOT);
        case SL_TOK_RETURN:
            tok = next_token(ps);
            if(!(ps->scope->flags & SL_PF_CAN_RETURN)) {
                error(ps, sl_make_cstring(ps->vm, "Can't return outside of a method or lambda"), tok);
            }
            switch(peek_token(ps)->type) {
                case SL_TOK_SEMICOLON:
                case SL_TOK_CLOSE_BRACE:
                case SL_TOK_CLOSE_TAG:
                /* in these case we want to allow for postfix control structures: */
                case SL_TOK_IF:
                case SL_TOK_UNLESS:
                    return sl_make_unary_node(ps, sl_make_immediate_node(ps, ps->vm->lib.nil), SL_NODE_RETURN);
                default:
                    return sl_make_unary_node(ps, low_precedence_logical_expression(ps), SL_NODE_RETURN);
            }
            break;
        case SL_TOK_THROW:
            next_token(ps);
            return sl_make_unary_node(ps, low_precedence_logical_expression(ps), SL_NODE_THROW);
        case SL_TOK_USE:
            return use_expression(ps);
        default:
            return power_expression(ps);
    }
}

static sl_node_base_t*
mul_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = unary_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    while(peek_token(ps)->type == SL_TOK_TIMES || peek_token(ps)->type == SL_TOK_DIVIDE ||
            peek_token(ps)->type == SL_TOK_MOD) {
        tok = next_token(ps);
        right = unary_expression(ps);
        left = sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
    }
    return left;
}

static sl_node_base_t*
add_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = mul_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    while(peek_token(ps)->type == SL_TOK_PLUS || peek_token(ps)->type == SL_TOK_MINUS) {
        tok = next_token(ps);
        right = mul_expression(ps);
        left = sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
    }
    return left;
}

static sl_node_base_t*
shift_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = add_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    while(peek_token(ps)->type == SL_TOK_SHIFT_LEFT || peek_token(ps)->type == SL_TOK_SHIFT_RIGHT) {
        tok = next_token(ps);
        right = add_expression(ps);
        left = sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
    }
    return left;
}

static sl_node_base_t*
bitwise_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = shift_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    while(1) {
        switch(peek_token(ps)->type) {
            case SL_TOK_PIPE:
            case SL_TOK_AMP:
            case SL_TOK_CARET:
                tok = next_token(ps);
                right = shift_expression(ps);
                left = sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
                break;
            default:
                return left;
        }
    }
}

static sl_node_base_t*
relational_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = bitwise_expression(ps);
    sl_token_t* tok;
    sl_node_base_t* right;
    switch(peek_token(ps)->type) {
        case SL_TOK_DBL_EQUALS:
        case SL_TOK_NOT_EQUALS:
        case SL_TOK_LT:
        case SL_TOK_GT:
        case SL_TOK_LTE:
        case SL_TOK_GTE:
        case SL_TOK_SPACESHIP:
        case SL_TOK_TILDE:
            tok = next_token(ps);
            right = bitwise_expression(ps);
            return sl_make_send_node(ps, left, sl_intern2(ps->vm, tok->str), 1, &right);
        default:
            return left;
    }
}

static sl_node_base_t*
logical_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = relational_expression(ps);
    sl_node_base_t* right;
    while(1) {
        switch(peek_token(ps)->type) {
            case SL_TOK_OR:
                next_token(ps);
                right = relational_expression(ps);
                left = sl_make_binary_node(ps, left, right, SL_NODE_OR);
                break;
            case SL_TOK_AND:
                next_token(ps);
                right = relational_expression(ps);
                left = sl_make_binary_node(ps, left, right, SL_NODE_AND);
                break;
            default:
                return left;
        }
    }
}

static sl_node_base_t*
range_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = logical_expression(ps);
    switch(peek_token(ps)->type) {
        case SL_TOK_RANGE:
            next_token(ps);
            return sl_make_range_node(ps, left, logical_expression(ps), 0);
        case SL_TOK_RANGE_EX:
            next_token(ps);
            return sl_make_range_node(ps, left, logical_expression(ps), 1);
        default:
            return left;
    }
}

static sl_node_base_t*
assignment_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = range_expression(ps);
    sl_token_t* tok = peek_token(ps);
    char* op_method;
    switch(tok->type) {
        case SL_TOK_EQUALS:             op_method = NULL; break;
        case SL_TOK_ASSIGN_PLUS:        op_method = "+";  break;
        case SL_TOK_ASSIGN_MINUS:       op_method = "-";  break;
        case SL_TOK_ASSIGN_POW:         op_method = "**"; break;
        case SL_TOK_ASSIGN_TIMES:       op_method = "*";  break;
        case SL_TOK_ASSIGN_DIVIDE:      op_method = "/";  break;
        case SL_TOK_ASSIGN_MOD:         op_method = "%";  break;
        case SL_TOK_ASSIGN_PIPE:        op_method = "|";  break;
        case SL_TOK_ASSIGN_AMP:         op_method = "&";  break;
        case SL_TOK_ASSIGN_CARET:       op_method = "^";  break;
        case SL_TOK_ASSIGN_OR:          op_method = "||"; break;
        case SL_TOK_ASSIGN_AND:         op_method = "&&"; break;
        case SL_TOK_ASSIGN_SHIFT_LEFT:  op_method = "<<"; break;
        case SL_TOK_ASSIGN_SHIFT_RIGHT: op_method = ">>"; break;
        
        default:
            return left;
    }
    switch(left->type) {
        case SL_NODE_VAR:
        case SL_NODE_IVAR:
        case SL_NODE_CVAR:
            next_token(ps);
            left = sl_make_simple_assign_node(ps, (sl_node_var_t*)left, assignment_expression(ps), op_method);
            break;
        case SL_NODE_SEND:
            next_token(ps);
            left = sl_make_assign_send_node(ps, (sl_node_send_t*)left, assignment_expression(ps), op_method);
            break;
        case SL_NODE_CONST:
            /*  compound assignment makes no sense on constants, so error
                if the assignment operator is anything except '=': */
            expect_token(ps, SL_TOK_EQUALS);
            left = sl_make_assign_const_node(ps, (sl_node_const_t*)left, assignment_expression(ps));
            break;
        case SL_NODE_ARRAY:
            /*  compound assignment makes no sense on arrays, so error if
                the assignment operator is anything except '=': */
            expect_token(ps, SL_TOK_EQUALS);
            left = sl_make_assign_array_node(ps, (sl_node_array_t*)left, assignment_expression(ps));
            break;
        default:
            break;
    }
    return left;
}

static sl_node_base_t*
low_precedence_not_expression(sl_parse_state_t* ps)
{
    if(peek_token(ps)->type == SL_TOK_LP_NOT) {
        return sl_make_unary_node(ps, assignment_expression(ps), SL_NODE_NOT);
    } else {
        return assignment_expression(ps);
    }
}

static sl_node_base_t*
low_precedence_logical_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = low_precedence_not_expression(ps);
    if(peek_token(ps)->type == SL_TOK_LP_AND) {
        next_token(ps);
        left = sl_make_binary_node(ps, left, low_precedence_logical_expression(ps), SL_NODE_AND);
    }
    if(peek_token(ps)->type == SL_TOK_LP_OR) {
        next_token(ps);
        left = sl_make_binary_node(ps, left, low_precedence_logical_expression(ps), SL_NODE_OR);
    }
    return left;
}

static sl_node_base_t*
postfix_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* expr = low_precedence_logical_expression(ps);
    if(token(ps)->type == SL_TOK_CLOSE_BRACE && peek_token(ps)->line > token(ps)->line) {
        return expr; // postfix expressions are only valid on the same line
    }
    while(1) {
        switch(peek_token(ps)->type) {
            case SL_TOK_IF:
                next_token(ps);
                expr = sl_make_if_node(ps, low_precedence_logical_expression(ps), expr, NULL);
                break;
            case SL_TOK_UNLESS:
                next_token(ps);
                expr = sl_make_if_node(ps,
                    sl_make_unary_node(ps, low_precedence_logical_expression(ps), SL_NODE_NOT),
                    expr, NULL);
                break;
            default:
                return expr;
        }
    }
}

static sl_node_base_t*
expression(sl_parse_state_t* ps)
{
    return postfix_expression(ps);
}

static sl_node_base_t*
echo_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_ECHO_TAG);
    expr = expression(ps);
    expect_token(ps, SL_TOK_CLOSE_TAG);
    return sl_make_echo_node(ps, expr);
}

static sl_node_base_t*
echo_raw_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_RAW_ECHO_TAG);
    expr = expression(ps);
    expect_token(ps, SL_TOK_CLOSE_TAG);
    return sl_make_raw_echo_node(ps, expr);
}

static sl_node_base_t*
inline_raw(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node(ps);
    while(1) {
        switch(peek_token(ps)->type) {
            case SL_TOK_OPEN_ECHO_TAG:
                sl_seq_node_append(ps, seq, echo_tag(ps));
                break;
            case SL_TOK_OPEN_RAW_ECHO_TAG:
                sl_seq_node_append(ps, seq, echo_raw_tag(ps));
                break;
            case SL_TOK_RAW:
                sl_seq_node_append(ps, seq, sl_make_raw_node(ps, next_token(ps)));
                break;
            case SL_TOK_END:
                return (sl_node_base_t*)seq;
            case SL_TOK_OPEN_TAG:
                return (sl_node_base_t*)seq;
            default: break;
        }
    }
}

static sl_node_base_t*
statement(sl_parse_state_t* ps)
{
    sl_node_base_t* node;
    switch(peek_token(ps)->type) {
        case SL_TOK_CLOSE_TAG:
            next_token(ps);
            node = inline_raw(ps);
            if(peek_token(ps)->type != SL_TOK_END) {
                expect_token(ps, SL_TOK_OPEN_TAG);
            }
            return node;
        case SL_TOK_SEMICOLON:
            next_token(ps);
            return NULL;
        case SL_TOK_IF:
        case SL_TOK_UNLESS:
            return if_expression(ps);
        case SL_TOK_SWITCH:
            return switch_expression(ps);
        case SL_TOK_FOR:
            return for_expression(ps);
        case SL_TOK_WHILE:
        case SL_TOK_UNTIL:
            return while_expression(ps);
        case SL_TOK_DEF:
            return def_expression(ps);
        case SL_TOK_CLASS:
            return class_expression(ps);
        default:
            node = expression(ps);
            if(peek_token(ps)->type != SL_TOK_CLOSE_TAG
                && peek_token(ps)->type != SL_TOK_CLOSE_BRACE
                && token(ps)->type != SL_TOK_CLOSE_BRACE
                && peek_token(ps)->type != SL_TOK_END) {
                expect_token(ps, SL_TOK_SEMICOLON);
            }
            return node;
    }
}

static sl_node_base_t*
statements(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node(ps);
    sl_node_base_t* node;
    while(peek_token(ps)->type != SL_TOK_END) {
        node = statement(ps);
        if(node) {
            sl_seq_node_append(ps, seq, node);
        }
    }
    return (sl_node_base_t*)seq;
}

sl_node_base_t*
sl_parse(sl_vm_t* vm, sl_token_t* tokens, size_t token_count, uint8_t* filename)
{
    sl_parse_scope_t scope;
    sl_parse_state_t ps;
    ps.vm = vm;
    ps.tokens = tokens;
    ps.token_count = token_count;
    ps.current_token = 0;
    ps.filename = filename;
    ps.scope = &scope;
    ps.line = 0;
    scope.flags = SL_PF_SCOPE_CLOSURE;
    return statements(&ps);
}
