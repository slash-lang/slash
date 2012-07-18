#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "eval.h"
#include "lib/float.h"
#include "lib/number.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <gc.h>

/*
static sl_token_t*
token(sl_parse_state_t* ps)
{
    return &ps->tokens[ps->current_token - 1];
}
*/

static sl_token_t*
peek_token(sl_parse_state_t* ps)
{
    return &ps->tokens[ps->current_token];
}

static sl_token_t*
next_token(sl_parse_state_t* ps)
{
    return &ps->tokens[ps->current_token++];
}

static void
error(sl_parse_state_t* ps, char* fmt, ...)
{
    char buff[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buff, 1023, fmt, va);
    sl_throw_message2(ps->vm, ps->vm->lib.SyntaxError, buff);
}

static void
unexpected(sl_parse_state_t* ps, sl_token_t* tok)
{
    SLVAL err = sl_make_cstring(ps->vm, "Unexpected '");
    err = sl_string_concat(ps->vm, err, tok->str);
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, "'"));
    sl_throw(ps->vm, sl_make_error2(ps->vm, ps->vm->lib.SyntaxError, err));
}

static sl_node_base_t*
expression(sl_parse_state_t* ps);

static sl_node_base_t*
primary_expression(sl_parse_state_t* ps);

static sl_node_base_t*
statement(sl_parse_state_t* ps);

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
    expect_token(ps, SL_TOK_OPEN_BRACE);
    while(peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
        sl_seq_node_append(seq, statement(ps));
    }
    expect_token(ps, SL_TOK_CLOSE_BRACE);
    return (sl_node_base_t*)seq;
}

static sl_node_base_t*
if_expression(sl_parse_state_t* ps)
{
    sl_node_base_t *condition, *if_true, *if_false = NULL;
    expect_token(ps, SL_TOK_IF);
    condition = expression(ps);
    if_true = body_expression(ps);
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
    /* @TODO */
    (void)ps;
    return NULL;
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

static sl_node_base_t*
def_expression(sl_parse_state_t* ps)
{
    SLVAL name;
    sl_node_base_t* on;
    sl_node_base_t* body;
    sl_token_t* tok;
    size_t arg_count = 0, arg_cap = 2;
    sl_string_t** args = GC_MALLOC(sizeof(sl_string_t*) * arg_cap);
    expect_token(ps, SL_TOK_DEF);
    on = primary_expression(ps);
    if(peek_token(ps)->type == SL_TOK_DOT) {
        next_token(ps);
        tok = expect_token(ps, SL_TOK_IDENTIFIER);
        name = sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len);
    } else {
        if(on->type != SL_NODE_VAR) {
            expect_token(ps, SL_TOK_DOT);
        }
        name = sl_make_ptr((sl_object_t*)((sl_node_var_t*)on)->name);
        on = NULL;
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
            return sl_make_const_node(sl_make_immediate_node(ps->vm->lib.Object),
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_IDENTIFIER:
            tok = next_token(ps);
            node = sl_make_var_node(ps, SL_NODE_VAR, sl_eval_var,
                sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
            return node;
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
    size_t argc = 0, cap = 2;
    sl_node_base_t** argv = GC_MALLOC(sizeof(sl_node_base_t*) * cap);
    tok = expect_token(ps, SL_TOK_IDENTIFIER);
    id = sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len);
    if(peek_token(ps)->type != SL_TOK_OPEN_PAREN) {
        return sl_make_send_node(recv, id, 0, NULL);
    }
    expect_token(ps, SL_TOK_OPEN_PAREN);
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
    return sl_make_send_node(recv, id, argc, argv);
}

static sl_node_base_t*
call_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = primary_expression(ps);
    sl_token_t* tok;
    while(peek_token(ps)->type == SL_TOK_DOT
        || peek_token(ps)->type == SL_TOK_PAAMAYIM_NEKUDOTAYIM) {
        tok = next_token(ps);
        if(tok->type == SL_TOK_DOT) {
            left = send_expression(ps, left);
        } else {
            tok = expect_token(ps, SL_TOK_CONSTANT);
            left = sl_make_const_node(left, sl_make_string(ps->vm, tok->as.str.buff, tok->as.str.len));
        }
    }
    return left;
}

static sl_node_base_t*
power_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return call_expression(ps);
}

static sl_node_base_t*
unary_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return power_expression(ps);
}

static sl_node_base_t*
mul_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = unary_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    char* id;
    while(peek_token(ps)->type == SL_TOK_TIMES || peek_token(ps)->type == SL_TOK_DIVIDE ||
            peek_token(ps)->type == SL_TOK_MOD) {
        tok = next_token(ps);
        right = unary_expression(ps);
        switch(tok->type) {
            case SL_TOK_TIMES:  id = "*"; break;
            case SL_TOK_DIVIDE: id = "/"; break;
            case SL_TOK_MOD:    id = "%"; break;
            default: /* wtf? */ break;
        }
        left = sl_make_send_node(left, sl_make_cstring(ps->vm, id), 1, &right);
    }
    return left;
}

static sl_node_base_t*
add_expression(sl_parse_state_t* ps)
{
    sl_node_base_t* left = mul_expression(ps);
    sl_node_base_t* right;
    sl_token_t* tok;
    SLVAL id;
    while(peek_token(ps)->type == SL_TOK_PLUS || peek_token(ps)->type == SL_TOK_MINUS) {
        tok = next_token(ps);
        right = mul_expression(ps);
        id = sl_make_cstring(ps->vm, tok->type == SL_TOK_PLUS ? "+" : "-");
        left = sl_make_send_node(left, id, 1, &right);
    }
    return left;
}

static sl_node_base_t*
shift_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return add_expression(ps);
}

static sl_node_base_t*
bitwise_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return shift_expression(ps);
}

static sl_node_base_t*
relational_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return bitwise_expression(ps);
}

static sl_node_base_t*
logical_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return relational_expression(ps);
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
    sl_node_base_t* left = range_expression(ps);
    if(left->type == SL_NODE_SEND || left->type == SL_NODE_VAR ||
        left->type == SL_NODE_IVAR || left->type == SL_NODE_CVAR) {
        /* valid lvals */
        if(peek_token(ps)->type == SL_TOK_EQUALS) {
            next_token(ps);
            return sl_make_binary_node(left, assignment_expression(ps), SL_NODE_ASSIGN, sl_eval_assign);
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
            default:
                error(ps, "Should never happen");
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
        default:
            node = expression(ps);
            if(peek_token(ps)->type != SL_TOK_CLOSE_TAG
                && peek_token(ps)->type != SL_TOK_CLOSE_BRACE) {
                expect_token(ps, SL_TOK_SEMICOLON);
            }
            return node;
    }
}

static sl_node_base_t*
statements(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node();
    while(peek_token(ps)->type != SL_TOK_END) {
        sl_seq_node_append(seq, statement(ps));
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
