%{
    #include "lex.h"
%}

%option noyywrap
%option yylineno
%option reentrant
%option extra-type="sl_lex_state_t*"

%{
    #include <gc.h>
    
    #define ADD_TOKEN(tok) do { \
            if(yyextra->len + 1 >= yyextra->cap) { \
                yyextra->tokens = GC_REALLOC(yyextra->tokens, yyextra->cap *= 2); \
            } \
            yyextra->tokens[yyextra->len++] = (tok); \
        } while(0)
%}

%x SLASH

%%

<INITIAL>"<%="  { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_ECHO_TAG));       BEGIN(SLASH); }
<INITIAL>"<%!!" { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_RAW_ECHO_TAG));   BEGIN(SLASH); }
<INITIAL>"<%"   { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_TAG));            BEGIN(SLASH); }

<INITIAL>.      { sl_lex_append_to_raw(yyextra, yytext, 1); }

<SLASH>"%>"     { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_TAG));           BEGIN(INITIAL); }
<SLASH>"nil"    { ADD_TOKEN(sl_make_token(SL_TOK_NIL)); }
<SLASH>"true"   { ADD_TOKEN(sl_make_token(SL_TOK_TRUE)); }
<SLASH>"false"  { ADD_TOKEN(sl_make_token(SL_TOK_FALSE)); }
<SLASH>"self"   { ADD_TOKEN(sl_make_token(SL_TOK_SELF)); }

<SLASH>" "      { /* ignore */ }
<SLASH>\t       { /* ignore */ }
<SLASH>\n       { /* ignore */ }

<SLASH>.        { sl_lex_error(yyextra, yytext, yylineno); }

%%

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t len, size_t* token_count)
{
    yyscan_t yyscanner;
    YY_BUFFER_STATE buff_state;
    sl_lex_state_t ls;
    ls.vm = vm;
    ls.cap = 8;
    ls.len = 0;
    ls.tokens = GC_MALLOC(sizeof(sl_token_t) * ls.cap);
    ls.filename = filename;
    
    yylex_init_extra(&ls, &yyscanner);
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
