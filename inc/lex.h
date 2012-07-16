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
    SL_TOK_IN,
    SL_TOK_WHILE,
    SL_TOK_UNTIL,
    
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
    
    SL_TOK_PLUS,
    SL_TOK_MINUS,
    SL_TOK_TIMES,
    SL_TOK_DIVIDE,
    SL_TOK_MOD,
    
    SL_TOK_LP_OR,
    SL_TOK_LP_AND,
    SL_TOK_LP_NOT,
    
    SL_TOK_OR,
    SL_TOK_AND,
    SL_TOK_NOT,
    
    SL_TOK_DOT,
    SL_TOK_END
}
sl_token_type_t;

typedef struct sl_token {
    sl_token_type_t type;
    union {
        struct {
            uint8_t* buff;
            size_t len;
            size_t cap;
        } str;
        double dbl;
    } as;
}
sl_token_t;

typedef struct sl_lex_state {
    sl_vm_t* vm;
    sl_token_t* tokens;
    size_t cap;
    size_t len;
    uint8_t* filename;
}
sl_lex_state_t;

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t len, size_t* token_count);

/* helper functions defined in lex_helper.c */

sl_token_t
sl_make_token(sl_token_type_t type);

sl_token_t
sl_make_string_token(sl_token_type_t type, char* buff, size_t len);

sl_token_t
sl_make_float_token(char* text);

void
sl_lex_append_to_raw(sl_lex_state_t* st, char* buff, size_t len);

void
sl_lex_error(sl_lex_state_t* st, char* text, int lineno);

#endif
