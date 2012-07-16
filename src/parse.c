#include "lex.h"
#include "ast.h"
#include "parse.h"
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
        error(ps, "Unexpected token: @TODO");
    }
    return tok;
}

static sl_node_base_t*
expression(sl_parse_state_t* ps)
{
    /* @TODO */
    (void)ps;
    return NULL;
}

static sl_node_base_t*
echo_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_ECHO_TAG);
    expr = expression(ps);
    return NULL;
    /* @TODO */
}

static sl_node_base_t*
echo_raw_tag(sl_parse_state_t* ps)
{
    sl_node_base_t* expr;
    expect_token(ps, SL_TOK_OPEN_RAW_ECHO_TAG);
    expr = expression(ps);
    return NULL;
    /* @TODO */
}

static sl_node_base_t*
inline_raw(sl_parse_state_t* ps)
{
    sl_node_seq_t* seq = sl_make_seq_node();
    expect_token(ps, SL_TOK_CLOSE_TAG);
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
