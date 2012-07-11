#ifndef SL_LEXER_H
#define SL_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include "vm.h"

typedef enum sl_token_type {
    SL_TOK_OPEN_TAG,
    SL_TOK_OPEN_ECHO_TAG,
    SL_TOK_OPEN_RAW_ECHO_TAG,
    SL_TOK_CLOSE_TAG,
    SL_TOK_RAW,
    
    SL_TOK_WHITESPACE,
    SL_TOK_IDENTIFIER,
    SL_TOK_IVAR,
    SL_TOK_CVAR,
    
    SL_TOK_STRING,
    SL_TOK_INTEGER,
    SL_TOK_FLOAT,
    SL_TOK_NIL,
    SL_TOK_TRUE,
    SL_TOK_FALSE,
    SL_TOK_SELF,
    
    SL_TOK_CLASS,
    SL_TOK_EXTENDS,
    SL_TOK_DEF,
    SL_TOK_IF,
    SL_TOK_ELSE,
    SL_TOK_UNLESS,
    SL_TOK_FOR,
    SL_TOK_WHILE,
    SL_TOK_IN,
    
    SL_TOK_OPEN_PAREN,
    SL_TOK_CLOSE_PAREN,
    SL_TOK_OPEN_BRACKET,
    SL_TOK_CLOSE_BRACKET,
    SL_TOK_OPEN_BRACE,
    SL_TOK_CLOSE_BRACE,
    
    SL_TOK_COMMA,
    SL_TOK_EQUALS,
    SL_TOK_DBL_EQUALS,
    SL_TOK_LT,
    SL_TOK_LTE,
    SL_TOK_GT,
    SL_TOK_GTE,
    
    SL_TOK_DOT
}
sl_token_type_t;

typedef struct sl_token {
    sl_token_type_t type;
    union {
        struct {
            uint8_t* buff;
            size_t len;
        } str;
        struct {
            double n;
        } dbl;
    } as;
}
sl_token_t;

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t len, size_t* token_count);

#endif