%option noyywrap
%option reentrant
%option extra-type="sl_token_buff_t*"

%{
    #include "lex.h"
    #include <gc.h>
    
    typedef struct sl_token_buff {
        sl_vm_t* vm;
        sl_token_t* tokens;
        size_t cap;
        size_t len;
    } sl_token_buff_t;
    
    #define ADD_TOKEN(tok) do { \
            if(yyextra->len + 1 >= yyextra->cap) { \
                yyextra->tokens = GC_REALLOC(yyextra->tokens, yyextra->cap *= 2); \
            } \
            yyextra->tokens[yyextra->len++] = (tok); \
        } while(0)
%}

%%

"nil"       { ADD_TOKEN(sl_make_token(SL_TOK_NIL)); }
"true"      { ADD_TOKEN(sl_make_token(SL_TOK_TRUE)); }
"false"     { ADD_TOKEN(sl_make_token(SL_TOK_FALSE)); }
"self"      { ADD_TOKEN(sl_make_token(SL_TOK_SELF)); }

.           { sl_throw_message2(yyextra->vm, yyextra->vm->lib.SyntaxError, "Illegal character"); }

%%

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t len, size_t* token_count)
{
    yyscan_t yyscanner;
    YY_BUFFER_STATE buff_state;
    sl_token_buff_t token_buff;
    token_buff.vm = vm;
    token_buff.cap = 8;
    token_buff.len = 0;
    token_buff.tokens = GC_MALLOC(sizeof(sl_token_t) * token_buff.cap);
    
    yylex_init_extra(&token_buff, &yyscanner);
    buff_state = yy_scan_bytes((char*)buff, len, yyscanner);
    yylex(yyscanner);
    yy_delete_buffer(buff_state, yyscanner);
    yylex_destroy(yyscanner);
    
    (void)vm;
    (void)filename;
    (void)buff;
    (void)len;
    (void)token_count;
    return NULL;
}
