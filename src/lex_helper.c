#include "lex.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

sl_token_t
sl_make_token(sl_token_type_t type)
{
    sl_token_t token;
    token.type = type;
    return token;
}

sl_token_t
sl_make_string_token(sl_token_type_t type, char* buff, size_t len)
{
    sl_token_t token;
    size_t cap = len < 4 ? 4 : len;
    token.type = type;
    token.as.str.buff = GC_MALLOC(cap);
    token.as.str.len = len;
    token.as.str.cap = cap;
    memcpy(token.as.str.buff, buff, len);
    return token;
}

sl_token_t
sl_make_float_token(char* text)
{
    sl_token_t token;
    token.type = SL_TOK_FLOAT;
    token.as.dbl = atof(text);
    return token;
}

void
sl_lex_append_to_raw(sl_lex_state_t* st, char* buff, size_t len)
{
    sl_token_t* raw_token;
    if(st->len == 0 || st->tokens[st->len - 1].type != SL_TOK_RAW) {
        if(st->cap + 1 >= st->len) {
            st->tokens = GC_REALLOC(st->tokens, st->cap *= 2);
        }
        st->tokens[st->len++] = sl_make_string_token(SL_TOK_RAW, "", 0);
    }
    raw_token = &st->tokens[st->len - 1];
    if(raw_token->as.str.cap < raw_token->as.str.len + len) {
        raw_token->as.str.buff = GC_REALLOC(raw_token->as.str.buff, raw_token->as.str.cap *= 2);
    }
    memcpy(raw_token->as.str.buff + raw_token->as.str.len, buff, len);
    raw_token->as.str.len += len;
}

void
sl_lex_error(sl_lex_state_t* st, char* text, int lineno)
{
    size_t filename_len = strlen((char*)st->filename);
    char* buff = GC_MALLOC(128 + filename_len);
    sprintf(buff, "Unexpected character '%c' in %s, line %d", text[0] /* todo utf 8 fix :\ */, st->filename, lineno);
    sl_throw_message2(st->vm, st->vm->lib.SyntaxError, buff);
}
