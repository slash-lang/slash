%{
    #include "lex.h"
%}

%option noyywrap yylineno reentrant nounistd never-interactive
%option extra-type="sl_lex_state_t*"

%{
    #include <gc.h>
    
    #define ADD_TOKEN(tok) do { \
            if(yyextra->len + 2 >= yyextra->cap) { \
                yyextra->tokens = GC_REALLOC(yyextra->tokens, sizeof(sl_token_t) * (yyextra->cap *= 2)); \
            } \
            yyextra->tokens[yyextra->len++] = (tok); \
        } while(0)
%}

%x SLASH

/* after each keyword, put '/{KW}' to look ahead for a non-identifier char */
KW  [^a-zA-Z_0-9]

%%

<INITIAL>"<%="      { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_ECHO_TAG));       BEGIN(SLASH); }
<INITIAL>"<%!!"     { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_RAW_ECHO_TAG));   BEGIN(SLASH); }
<INITIAL>"<%"       { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_TAG));            BEGIN(SLASH); }

<INITIAL>.          { sl_lex_append_to_raw(yyextra, yytext, 1); }

<SLASH>"%>"         { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_TAG));           BEGIN(INITIAL); }

<SLASH>[0-9]+"e"[+-]?[0-9]+                 { ADD_TOKEN(sl_make_float_token(yytext)); }
<SLASH>[0-9]+("."[0-9]+)("e"[+-]?[0-9]+)?   { ADD_TOKEN(sl_make_float_token(yytext)); }

<SLASH>[0-9]+           { ADD_TOKEN(sl_make_string_token(SL_TOK_INTEGER, yytext, yyleng)); }

<SLASH>"nil"/{KW}       { ADD_TOKEN(sl_make_token(SL_TOK_NIL)); }
<SLASH>"true"/{KW}      { ADD_TOKEN(sl_make_token(SL_TOK_TRUE)); }
<SLASH>"false"/{KW}     { ADD_TOKEN(sl_make_token(SL_TOK_FALSE)); }
<SLASH>"self"/{KW}      { ADD_TOKEN(sl_make_token(SL_TOK_SELF)); }
<SLASH>"class"/{KW}     { ADD_TOKEN(sl_make_token(SL_TOK_CLASS)); }
<SLASH>"extends"/{KW}   { ADD_TOKEN(sl_make_token(SL_TOK_EXTENDS)); }
<SLASH>"def"/{KW}       { ADD_TOKEN(sl_make_token(SL_TOK_DEF)); }
<SLASH>"if"/{KW}        { ADD_TOKEN(sl_make_token(SL_TOK_IF)); }
<SLASH>"else"/{KW}      { ADD_TOKEN(sl_make_token(SL_TOK_ELSE)); }
<SLASH>"unless"/{KW}    { ADD_TOKEN(sl_make_token(SL_TOK_UNLESS)); }
<SLASH>"for"/{KW}       { ADD_TOKEN(sl_make_token(SL_TOK_FOR)); }
<SLASH>"in"/{KW}        { ADD_TOKEN(sl_make_token(SL_TOK_IN)); }
<SLASH>"while"/{KW}     { ADD_TOKEN(sl_make_token(SL_TOK_WHILE)); }
<SLASH>"until"/{KW}     { ADD_TOKEN(sl_make_token(SL_TOK_UNTIL)); }

<SLASH>"("          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_PAREN)); }
<SLASH>")"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_PAREN)); }
<SLASH>"["          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_BRACKET)); }
<SLASH>"]"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_BRACKET)); }
<SLASH>"{"          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_BRACE)); }
<SLASH>"}"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_BRACE)); }

<SLASH>","          { ADD_TOKEN(sl_make_token(SL_TOK_COMMA)); }
<SLASH>"=="         { ADD_TOKEN(sl_make_token(SL_TOK_DBL_EQUALS)); }
<SLASH>"="          { ADD_TOKEN(sl_make_token(SL_TOK_EQUALS)); }
<SLASH>"<="         { ADD_TOKEN(sl_make_token(SL_TOK_LTE)); }
<SLASH>"<"          { ADD_TOKEN(sl_make_token(SL_TOK_LT)); }
<SLASH>">="         { ADD_TOKEN(sl_make_token(SL_TOK_GTE)); }
<SLASH>">"          { ADD_TOKEN(sl_make_token(SL_TOK_GT)); }

<SLASH>"+"          { ADD_TOKEN(sl_make_token(SL_TOK_PLUS)); }
<SLASH>"-"          { ADD_TOKEN(sl_make_token(SL_TOK_MINUS)); }
<SLASH>"*"          { ADD_TOKEN(sl_make_token(SL_TOK_TIMES)); }
<SLASH>"/"          { ADD_TOKEN(sl_make_token(SL_TOK_DIVIDE)); }
<SLASH>"%"          { ADD_TOKEN(sl_make_token(SL_TOK_MOD)); }

<SLASH>"."          { ADD_TOKEN(sl_make_token(SL_TOK_DOT)); }

<SLASH>[ \t\r\n]    { /* ignore */ }

<SLASH>.            { sl_lex_error(yyextra, yytext, yylineno); }

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
    
    ls.tokens[ls.len++].type = SL_TOK_CLOSE_TAG;
    
    yylex_init_extra(&ls, &yyscanner);
    buff_state = yy_scan_bytes((char*)buff, len, yyscanner);
    yylex(yyscanner);
    yy_delete_buffer(buff_state, yyscanner);
    yylex_destroy(yyscanner);
    
    ls.tokens[ls.len++].type = SL_TOK_END;
    *token_count = ls.len;
    return ls.tokens;
}
