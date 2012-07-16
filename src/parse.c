#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "eval.h"
#include "lib/float.h"
#include "lib/number.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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

static sl_token_t*
expect_token(sl_parse_state_t* ps, sl_token_type_t type)
{
    sl_token_t* tok = next_token(ps);
    if(tok->type != type) {
        error(ps, "Unexpected token: @TODO [expect_token]");
    }
    return tok;
}

static sl_node_base_t*
if_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    (void)ps;
    return NULL;
}

static sl_node_base_t*
while_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    (void)ps;
    return NULL;
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
    /* @TODO */
    (void)ps;
    return NULL;
}

static sl_node_base_t*
def_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    (void)ps;
    return NULL;
}

static sl_node_base_t*
primary_expression(sl_parse_state_t* ps)
{
    sl_token_t* tok;
    switch(peek_token(ps)->type) {
        case SL_TOK_INTEGER:
            tok = next_token(ps);
            return sl_make_immediate_node(sl_number_parse(ps->vm, tok->as.str.buff, tok->as.str.len));
        case SL_TOK_FLOAT:
            return sl_make_immediate_node(sl_make_float(ps->vm, next_token(ps)->as.dbl));
        default:
            error(ps, "Unexpected token: @TODO [primary_expression]");
            return NULL;
    }
}

static sl_node_base_t*
call_expression(sl_parse_state_t* ps)
{
    /* @TODO */
    return primary_expression(ps);
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
        left = sl_make_send_node(ps, left, id, 1, &right);
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
        left = sl_make_send_node(ps, left, tok->type == SL_TOK_PLUS ? "+" : "-", 1, &right);
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
    sl_token_type_t type = peek_token(ps)->type;
    switch(type) {
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
            return low_precedence_logical_expression(ps);
    }
    return NULL;
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
                next_token(ps);
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
            error(ps, "wtf lol");
            return NULL;
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
