#include <stdarg.h>
#include <stdio.h>
#include <gc.h>
#include "lexer.h"
#include "error.h"

typedef struct lexer_state {
    size_t len;
    size_t cap;
    sl_token_t* tokens;
    sl_vm_t* vm;
    uint8_t* filename;
    size_t line;
    size_t col;
}
ls_t;

static void
error(ls_t* LS, char* fmt, ...)
{
    va_list va;
    char buff[512], buff2[1024];
    va_start(va, fmt);
    vsnprintf(buff, 511, fmt, va);
    buff[511] = 0;
    snprintf(buff2, 1023, "%s in %s at line %lu, col %lu", buff, LS->filename, LS->line, LS->col);
    buff2[1023] = 0;
    sl_throw_message2(LS->vm, LS->vm->lib.SyntaxError, buff2);
}

static uint32_t*
utf8_to_utf32(ls_t* LS, uint8_t* buff, size_t len, size_t* out_len)
{
    uint32_t* out = GC_MALLOC(sizeof(uint32_t) * len);
    size_t str_len = 0;
    size_t i;
    uint32_t c;
    for(i = 0; i < len; i++) {
        if((buff[i] & 0x80) == 0x00) { /* 1 byte char */
            out[str_len++] = buff[i];
            continue;
        }
        if((buff[i] & 0xe0) == 0xc0) { /* 2 byte char */
            if(i + 1 == len) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 1] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            c  = (buff[i]     & 0x1f) << 6;
            c |= (buff[i + 1] & 0x3f) << 0;
            if(c < 128) error(LS, "Invalid UTF-8 sequence");
            out_len[str_len++] = c;
            continue;
        }
        if((buff[i] & 0xf0) == 0xe0) { /* 3 byte char */
            if(i + 2 == len) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 1] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 2] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            c  = (buff[i]     & 0x0f) << 12;
            c |= (buff[i + 1] & 0x3f) << 6;
            c |= (buff[i + 2] & 0x3f) << 0;
            if(c < 2048) error(LS, "Invalid UTF-8 sequence");
            out_len[str_len++] = c;
            continue;
        }
        if((buff[i] & 0xf8) == 0xf0) { /* 4 byte char */
            if(i + 3 == len) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 1] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 2] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            if((buff[i + 3] & 0xc0) != 0x80) error(LS, "Invalid UTF-8 sequence");
            c  = (buff[i]     & 0x07) << 18;
            c |= (buff[i + 1] & 0x3f) << 12;
            c |= (buff[i + 2] & 0x3f) << 6;
            c |= (buff[i + 3] & 0x3f) << 0;
            if(c < 2048) error(LS, "Invalid UTF-8 sequence");
            out_len[str_len++] = c;
            continue;
        }
        error(LS, "Invalid UTF-8 sequence");
    }
    *out_len = str_len;
    return out;
}

static size_t
utf32_char_to_utf8(ls_t* LS, uint32_t u32c, uint8_t* u8buff)
{
    if(u32c < 0x80) {
        u8buff[0] = (uint8_t)u32c;
        return 1;
    }
    if(u32c < 0x800) {
        u8buff[0] = 0xc0 | ((u32c >>  6) & 0x1f);
        u8buff[1] = 0x80 | ((u32c >>  0) & 0x3f);
        return 2;
    }
    if(u32c < 0x10000) {
        u8buff[0] = 0xe0 | ((u32c >> 12) & 0x0f);
        u8buff[1] = 0x80 | ((u32c >>  6) & 0x3f);
        u8buff[2] = 0x80 | ((u32c >>  0) & 0x3f);
        return 3;
    }
    if(u32c < 0x200000) {
        u8buff[0] = 0xe0 | ((u32c >> 18) & 0x0f);
        u8buff[1] = 0x80 | ((u32c >> 12) & 0x3f);
        u8buff[2] = 0x80 | ((u32c >>  6) & 0x3f);
        u8buff[3] = 0x80 | ((u32c >>  0) & 0x3f);
        return 4;
    }
    sl_throw_message2(LS->vm, LS->vm->lib.EncodingError, "UTF-32 character out of UTF-8 range");
    return 0;
}

uint8_t*
utf32_to_utf8(ls_t* LS, uint32_t* u32, size_t u32_len, size_t* u8_len)
{
    size_t cap = 8;
    size_t len;
    uint8_t* buff = GC_MALLOC(cap);
    size_t i;
    for(i = 0; i < u32_len; i++) {
        if(cap + 5 >= len) {
            buff = GC_REALLOC(buff, cap *= 2);
        }
        len += utf32_char_to_utf8(LS, u32[i], buff + len);
    }
    buff[len] = 0;
    *u8_len = len;
    return buff;
}

#define ADD_TOKEN(tok) do { \
    if(LS->len + 1 == LS->cap) { \
        LS->tokens = GC_REALLOC(sizeof(sl_token_t) * (LS->cap *= 2)); \
    } \
    LS->tokens[LS->len++] = (tok); \
} while(0)

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t buff_len, size_t* token_count)
{
    size_t i = 0, j = 0;
    uint32_t* chars;
    uint32_t c;
    sl_token_t tok;
    size_t len;
    
    int in_raw = 1;
    
    ls_t LS;
    LS.len = 0;
    LS.cap = 8;
    LS.tokens = GC_MALLOC(sizeof(sl_token_t) * LS.cap);
    LS.vm = vm;
    LS.filename = filename;
    LS.line = 1;
    LS.col = 0;
    
    chars = utf8_to_utf32(&LS, buff, buff_len, &len);
    
    for(i = 0; i < len; i++) {
        c = chars[i];
        if(in_raw) {
            if(c == '<') {
                if(i + 1 < len && chars[i + 1] == '%') {
                    tok.type = SL_TOK_RAW;
                    tok.as.str.len = i - j;
                    tok.as.str.buff = GC_MALLOC(i - j);
                }
            }
            continue;
        }
    }
    
    *token_count = LS.len;
    return LS.tokens;
}