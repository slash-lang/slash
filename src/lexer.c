#include <stdarg.h>
#include <stdio.h>
#include <gc.h>
#include "lexer.h"
#include "error.h"
#include "utf8.h"

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

/*
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
*/

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
    
    chars = sl_utf8_to_utf32(vm, buff, buff_len, &len);
    
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