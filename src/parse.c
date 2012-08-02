#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "eval.h"
#include "lib/float.h"
#include "lib/number.h"
#include "string.h"
#include "object.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <gc.h>

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
unexpected(sl_parse_state_t* ps, sl_token_t* tok)
{
    SLVAL err;
    if(tok->type != SL_TOK_END) {
        err = sl_make_cstring(ps->vm, "Unexpected '");
        err = sl_string_concat(ps->vm, err, tok->str);
        err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, "' in "));
    } else {
        err = sl_make_cstring(ps->vm, "Unexpected end of file in");
    }    
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, (char*)ps->filename));
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, ", line "));
    err = sl_string_concat(ps->vm, err, sl_to_s(ps->vm, sl_make_int(ps->vm, tok->line)));
    sl_throw(ps->vm, sl_make_error2(ps->vm, ps->vm->lib.SyntaxError, err));
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
low_precedence_logical_expression(sl_parse_state_t* ps);

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
    sl_node_seq_t* seq = sl_make_seq_node();
    sl_node_base_t* node;
    expect_token(ps, SL_TOK_OPEN_BRACE);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        node = statement(ps);
        if(node) {
            sl_seq_node_append(seq, node);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_BRACE);
    return (sl_node_base_t*)seq;
}

static sl_node_base_t*
if_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *condition, *if_true, *if_false = NULL;
    if(peek_token(ps)->type == SL_TOK_ELSIF) {
        expect_token(ps, SL_TOK_ELSIF);
    } else {
        expect_token(ps, SL_TOK_IF);
    }
    condition = expression(ps);
    if_true = body_expression(ps);
    if(peek_token(ps)->type == SL_TOK_ELSIF) {
        if_false = if_expression(ps);
    }
    if(peek_token(ps)->type == SL_TOK_ELSE) {
        next_token(ps);
        if_false = body_expression(ps);
    }
    return sl_make_if_node(condition, if_true, if_false);
}

static sl_node_base_t*
while_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *condition, *body;
    int until = 0;
    if(peek_token(ps)->type == SL_TOK_UNTIL) {
        next_token(ps);
        until = 1;
    } else {
        expect_token(ps, SL_TOK_WHILE);
    }
    condition = expression(ps);
    if(until) {
        condition = sl_make_unary_node(condition, SL_NODE_NOT, sl_eval_not);
    }
    body = body_expression(ps);
    return sl_make_while_node(condition, body);
}

static sl_node_base_t*
for_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *lval, *expr, *body, *else_body = NULL;
    sl_node_seq_t* seq_lval;
    sl_token_t* tok;
    expect_token(ps, SL_TOK_FOR);
    /* save current token to allow rewinding and erroring */
    tok = peek_token(ps);
    lval = primary_expression(ps);
    if(!sl_node_is_lval(lval)) {
        unexpected(ps, tok);
    }
    if(peek_token(ps)->type == SL_TOK_COMMA) {
        seq_lval = sl_make_seq_node();
        seq_lval->nodes[seq_lval->node_count++] = lval;
        while(peek_token(ps)->type == SL_TOK_COMMA) {
            next_token(ps);
            if(seq_lval->node_count == seq_lval->node_capacity) {
                seq_lval->node_capacity *= 2;
                seq_lval->nodes = GC_REALLOC(seq_lval->nodes, sizeof(sl_node_base_t*) * seq_lval->node_capacity);
            }
            tok = peek_token(ps);
            lval = primary_expression(ps);
            if(!sl_node_is_lval(lval)) {
                unexpected(ps, tok);
            }
            seq_lval->nodes[seq_lval->node_count++] = lval;
        }
        lval = sl_make_array_node(seq_lval->node_count, seq_lval->nodes);
    }
    expect_token(ps, SL_TOK_IN);
    expr = expression(ps);
    body = body_expression(ps);
    if(peek_token(ps)->type == SL_TOK_ELSE) {
        next_token(ps);
        else_body = body_expression(ps);
    }
    return sl_make_for_node(lval, expr, body, else_body);
}

static sl_node_base_t*
class_expression(sl_parse_state_t* ps)
{
    SLVAL name;
    sl_node_base_t *extends, *body;
    sl_token_t* tok;
    expect_token(ps, SL_TOK_CLASS);
    tok = expect_token(ps, SL_TOK_CONSTANT);
    name = sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len);
    if(peek_token(ps)->type == SL_TOK_EXTENDS) {
        next_token(ps);
        extends = expression(ps);
    } else {
        extends = NULL;
    }
    body = body_expression(ps);
    return sl_make_class_node(ps, name, extends, body);
}

static SLVAL
def_expression_method_name(sl_parse_state_t* ps)
{
    switch(peek_token(ps)->type) {
        case SL_TOK_IDENTIFIER:
            return next_token(ps)->str;
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
        case SL_TOK_MINUS:
        case SL_TOK_POW:
        case SL_TOK_TIMES:
        case SL_TOK_DIVIDE:
        case SL_TOK_MOD:
        case SL_TOK_CARET:
        case SL_TOK_TILDE:
        case SL_TOK_BIT_AND:
        case SL_TOK_BIT_OR:
            return next_token(ps)->str;
        case SL_TOK_OPEN_BRACKET:
            if(peek_token_n(ps, 2)->type == SL_TOK_CLOSE_BRACKET) {
                next_token(ps);
                next_token(ps);
                return sl_make_cstring(ps->vm, "[]");
            }
        default:
            unexpected(ps, next_token(ps));
            return ps->vm->lib.nil; /* never reached */
    }
}

static sl_node_base_t*
def_expression(sl_parse_state_t* ps)
{
    SLVAL name = ps->vm->lib.nil;
    sl_node_base_t* on = NULL;
    sl_node_base_t* body;
    sl_token_t* tok;
    size_t arg_count = 0, arg_cap = 2;
    sl_string_t** args = GC_MALLOC(sizeof(sl_string_t*) * arg_cap);
    expect_token(ps, SL_TOK_DEF);
    switch(peek_token(ps)->type) {
        case SL_TOK_IDENTIFIER:
            if(peek_token_n(ps, 2)->type == SL_TOK_DOT) {
                on = sl_make_var_node(ps, SL_NODE_VAR, sl_eval_var, next_token(ps)->str);
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
        name = sl_string_concat(ps->vm, name, sl_make_cstring(ps->vm, "="));
    }
    if(peek_token(ps)->type != SL_TOK_OPEN_BRACE) {
        expect_token(ps, SL_TOK_OPEN_PAREN);
        while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            if(arg_count >= arg_cap) {
                arg_cap *= 2;
                args = GC_REALLOC(args, sizeof(sl_string_t*) * arg_cap);
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
    body = body_expression(ps);
    return sl_make_def_node(ps, name, on, arg_count, args, body);
}

static sl_node_base_t*
lambda_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* body;
    sl_token_t* tok;
    size_t arg_count = 0, arg_cap = 2;
    sl_string_t** args = GC_MALLOC(sizeof(sl_string_t*) * arg_cap);
    expect_token(ps, SL_TOK_LAMBDA);
    if(peek_token(ps)->type != SL_TOK_OPEN_BRACE) {
        expect_token(ps, SL_TOK_OPEN_PAREN);
        while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            if(arg_count >= arg_cap) {
                arg_cap *= 2;
                args = GC_REALLOC(args, sizeof(sl_string_t*) * arg_cap);
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
    body = body_expression(ps);
    return sl_make_lambda_node(arg_count, args, body);
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
    return sl_make_try_node(body, lval, catch_body);
}

static sl_node_base_t*
send_with_args_expression(sl_parse_state_t* ps, sl_node_base_t* recv, SLVAL id)
{
    size_t argc = 0, cap = 2;
    sl_node_base_t** argv = GC_MALLOC(sizeof(sl_node_base_t*) * cap);
    sl_token_t* tok = expect_token(ps, SL_TOK_OPEN_PAREN);
    while(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
        if(argc >= cap) {
            cap *= 2;
            argv = GC_REALLOC(argv, sizeof(sl_node_base_t*) * cap);
        }
        argv[argc++] = expression(ps);
        if(peek_token(ps)->type != SL_TOK_CLOSE_PAREN) {
            expect_token(ps, SL_TOK_COMMA);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_PAREN);
    return sl_make_send_node(ps, tok->line, recv, id, argc, argv);
}

static sl_node_base_t*
array_expression(sl_parse_state_t* ps)
{
    size_t count = 0, cap = 2;
    sl_node_base_t** nodes = GC_MALLOC(sizeof(sl_node_base_t*) * cap);
    expect_token(ps, SL_TOK_OPEN_BRACKET);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
        if(count >= cap) {
            cap *= 2;
            nodes = GC_REALLOC(nodes, sizeof(sl_node_base_t*) * cap);
        }
        nodes[count++] = expression(ps);
        if(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
            expect_token(ps, SL_TOK_COMMA);
        }
    }
    expect_token(ps, SL_TOK_CLOSE_BRACKET);
    return sl_make_array_node(count, nodes);
}

static sl_node_base_t*
dict_expression(sl_parse_state_t* ps)
{
    size_t count = 0, cap = 2;
    sl_node_base_t** keys = GC_MALLOC(sizeof(sl_node_base_t*) * cap);
    sl_node_base_t** vals = GC_MALLOC(sizeof(sl_node_base_t*) * cap);
    expect_token(ps, SL_TOK_OPEN_BRACE);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        if(count >= cap) {
            cap *= 2;
            keys = GC_REALLOC(keys, sizeof(sl_node_base_t*) * cap);
            vals = GC_REALLOC(vals, sizeof(sl_node_base_t*) * cap);
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
    return sl_make_dict_node(count, keys, vals);
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
primary_expression(sl_parse_state_t* ps)
{
    sl_token_t* tok;
    sl_node_base_t* node;
    switch(peek_token(ps)->type) {
        case SL_TOK_INTEGER:
            tok = next_token(ps);
            return sl_make_immediate_node(sl_number_parse(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_FLOAT:
            return sl_make_immediate_node(sl_make_float(ps->vm, next_token(ps)->as.dbl));
        case SL_TOK_STRING:
            tok = next_token(ps);
            return sl_make_immediate_node(sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_CONSTANT:
            tok = next_token(ps);
            return sl_make_const_node(NULL, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_IDENTIFIER:
            tok = next_token(ps);
            return sl_make_var_node(ps, SL_NODE_VAR, sl_eval_var,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_TRUE:
            next_token(ps);
            return sl_make_immediate_node(ps->vm->lib._true);
        case SL_TOK_FALSE:
            next_token(ps);
            return sl_make_immediate_node(ps->vm->lib._false);
        case SL_TOK_NIL:
            next_token(ps);
            return sl_make_immediate_node(ps->vm->lib.nil);
        case SL_TOK_SELF:
            next_token(ps);
            return sl_make_self_node();
        case SL_TOK_IVAR:
            tok = next_token(ps);
            node = sl_make_var_node(ps, SL_NODE_IVAR, sl_eval_ivar,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            return node;
        case SL_TOK_CVAR:
            tok = next_token(ps);
            node = sl_make_var_node(ps, SL_NODE_CVAR, sl_eval_cvar,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            return node;
        case SL_TOK_IF:
        case SL_TOK_UNLESS:
            return if_expression(ps);
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
        return sl_make_send_node(ps, tok->line, recv, id, 0, NULL);
    }
    return send_with_args_expression(ps, recv, id);
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
        left = send_with_args_expression(ps, sl_make_self_node(),
            sl_make_ptr((sl_object_t*)((sl_node_var_t*)left)->name));
    }
    while(peek_token(ps)->type == SL_TOK_DOT
        || peek_token(ps)->type == SL_TOK_PAAMAYIM_NEKUDOTAYIM
        || peek_token(ps)->type == SL_TOK_OPEN_BRACKET) {
        tok = next_token(ps);
        switch(tok->type) {
            case SL_TOK_DOT:
                left = send_expression(ps, left);
                break;
            case SL_TOK_PAAMAYIM_NEKUDOTAYIM:
                tok = expect_token(ps, SL_TOK_CONSTANT);
                left = sl_make_const_node(left, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
                break;
            case SL_TOK_OPEN_BRACKET:
                node_cap = 1;
                node_len = 0;
                nodes = GC_MALLOC(sizeof(SLVAL) * node_cap);
                while(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
                    if(node_len >= node_cap) {
                        node_cap *= 2;
                        nodes = GC_REALLOC(nodes, sizeof(SLVAL) * node_cap);
                    }
                    nodes[node_len++] = expression(ps);
                    if(peek_token(ps)->type != SL_TOK_CLOSE_BRACKET) {
                        expect_token(ps, SL_TOK_COMMA);
                    }
                }
                expect_token(ps, SL_TOK_CLOSE_BRACKET);
                left = sl_make_send_node(ps, tok->line, left, sl_make_cstring(ps->vm, "[]"), node_len, nodes);
                break;
            default: break;
        }
    }
    return left;
}

static sl_node_base_t*
power_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = call_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    if(peek_token(ps)->type == SL_TOK_POW) {
        tok = next_token(ps);
        right = power_expression(ps);
        left = sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
    }
    return left;
}

static sl_node_base_t*
unary_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    SLVAL id;
    sl_token_t* tok;
    switch(peek_token(ps)->type) {
        case SL_TOK_MINUS:
            tok = next_token(ps);
            id = sl_make_cstring(ps->vm, "negate");
            expr = unary_expression(ps);
            return sl_make_send_node(ps, tok->line, expr, id, 0, NULL);
        case SL_TOK_TILDE:
            tok = next_token(ps);
            id = sl_make_cstring(ps->vm, "~");
            expr = unary_expression(ps);
            return sl_make_send_node(ps, tok->line, expr, id, 0, NULL);
        case SL_TOK_NOT:
            next_token(ps);
            expr = unary_expression(ps);
            return sl_make_unary_node(expr, SL_NODE_NOT, sl_eval_not);
        case SL_TOK_RETURN:
            next_token(ps);
            switch(peek_token(ps)->type) {
                case SL_TOK_SEMICOLON:
                case SL_TOK_CLOSE_BRACE:
                case SL_TOK_CLOSE_TAG:
                    return sl_make_unary_node(sl_make_immediate_node(ps->vm->lib.nil), SL_NODE_RETURN, sl_eval_return);
                default:
                    return sl_make_unary_node(low_precedence_logical_expression(ps), SL_NODE_RETURN, sl_eval_return);
            }
            if(peek_token(ps)->type == SL_TOK_SEMICOLON || peek_token(ps)->type == SL_TOK_CLOSE_BRACE)
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
        left = sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
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
        left = sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
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
        left = sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
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
            case SL_TOK_BIT_OR:
            case SL_TOK_BIT_AND:
            case SL_TOK_CARET:
                tok = next_token(ps);
                right = shift_expression(ps);
                left = sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
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
            tok = next_token(ps);
            right = bitwise_expression(ps);
            return sl_make_send_node(ps, tok->line, left, tok->str, 1, &right);
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
                left = sl_make_binary_node(left, right, SL_NODE_OR, sl_eval_or);
                break;
            case SL_TOK_AND:
                next_token(ps);
                right = relational_expression(ps);
                left = sl_make_binary_node(left, right, SL_NODE_AND, sl_eval_and);
                break;
            default:
                return left;
        }
    }
}

static sl_node_base_t*
range_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return logical_expression(ps);
}

static sl_node_base_t*
assignment_expression(sl_parse_state_t* ps)
{
    sl_node_send_t* send;
    sl_node_base_t** argv;
    sl_node_base_t* left = range_expression(ps);
    sl_token_t* tok;
    if(peek_token(ps)->type == SL_TOK_EQUALS) {
        switch(left->type) {
            case SL_NODE_VAR:
                next_token(ps);
                left = sl_make_assign_var_node((sl_node_var_t*)left, assignment_expression(ps));
                break;
            case SL_NODE_IVAR:
                next_token(ps);
                left = sl_make_assign_ivar_node((sl_node_var_t*)left, assignment_expression(ps));
                break;
            case SL_NODE_CVAR:
                next_token(ps);
                left = sl_make_assign_cvar_node((sl_node_var_t*)left, assignment_expression(ps));
                break;
            case SL_NODE_SEND:
                tok = next_token(ps);
                send = (sl_node_send_t*)left;
                argv = GC_MALLOC(sizeof(sl_node_base_t*) * (send->arg_count + 1));
                memcpy(argv, send->args, sizeof(sl_node_base_t*) * send->arg_count);
                argv[send->arg_count] = assignment_expression(ps);
                left = sl_make_send_node(ps, tok->line, send->recv,
                    sl_string_concat(ps->vm, send->id, sl_make_cstring(ps->vm, "=")),
                    send->arg_count + 1, argv);
                break;
            case SL_NODE_CONST:
                next_token(ps);
                left = sl_make_assign_const_node((sl_node_const_t*)left, assignment_expression(ps));
                break;
            case SL_NODE_ARRAY:
                next_token(ps);
                left = sl_make_assign_array_node(ps, (sl_node_array_t*)left, assignment_expression(ps));
                break;
            default:
                break;
        }
    }
    return left;
}

static sl_node_base_t*
low_precedence_not_expression(sl_parse_state_t* ps)
{
    if(peek_token(ps)->type == SL_TOK_LP_NOT) {
        return sl_make_unary_node(assignment_expression(ps), SL_NODE_NOT, sl_eval_not);
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
        left = sl_make_binary_node(left, low_precedence_logical_expression(ps), SL_NODE_AND, sl_eval_and);
    }
    if(peek_token(ps)->type == SL_TOK_LP_OR) {
        next_token(ps);
        left = sl_make_binary_node(left, low_precedence_logical_expression(ps), SL_NODE_OR, sl_eval_or);
    }
    return left;
}

static sl_node_base_t*
expression(sl_parse_state_t* ps)
{
    return low_precedence_logical_expression(ps);
}

static sl_node_base_t*
echo_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_ECHO_TAG);
    expr = expression(ps);
    expect_token(ps, SL_TOK_CLOSE_TAG);
    return sl_make_echo_node(expr);
}

static sl_node_base_t*
echo_raw_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_RAW_ECHO_TAG);
    expr = expression(ps);
    expect_token(ps, SL_TOK_CLOSE_TAG);
    return sl_make_raw_echo_node(expr);
}

static sl_node_base_t*
inline_raw(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node();
    while(1) {
        switch(peek_token(ps)->type) {
            case SL_TOK_OPEN_ECHO_TAG:
                sl_seq_node_append(seq, echo_tag(ps));
                break;
            case SL_TOK_OPEN_RAW_ECHO_TAG:
                sl_seq_node_append(seq, echo_raw_tag(ps));
                break;
            case SL_TOK_RAW:
                sl_seq_node_append(seq, sl_make_raw_node(ps, next_token(ps)));
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
        default:
            node = expression(ps);
            if(peek_token(ps)->type != SL_TOK_CLOSE_TAG
                && peek_token(ps)->type != SL_TOK_CLOSE_BRACE
                && token(ps)->type != SL_TOK_CLOSE_BRACE) {
                expect_token(ps, SL_TOK_SEMICOLON);
            }
            return node;
    }
}

static sl_node_base_t*
statements(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node();
    sl_node_base_t* node;
    while(peek_token(ps)->type != SL_TOK_END) {
        node = statement(ps);
        if(node) {
            sl_seq_node_append(seq, node);
        }
    }
    return (sl_node_base_t*)seq;
}

sl_node_base_t*
sl_parse(sl_vm_t* vm, sl_token_t* tokens, size_t token_count, uint8_t* filename)
{
    sl_parse_state_t ps;
    ps.vm = vm;
    ps.tokens = tokens;
    ps.token_count = token_count;
    ps.current_token = 0;
    ps.filename = filename;
    return statements(&ps);
}
