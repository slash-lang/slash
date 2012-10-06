%{
    #include <slash/lex.h>
    #include <slash/string.h>
    #include <slash/error.h>
    #include <slash/mem.h>
%}

%option noyywrap yylineno reentrant nounistd never-interactive stack
%option extra-type="sl_lex_state_t*"

%{
    #define ADD_TOKEN(tok) do { \
            if(yyextra->len + 2 >= yyextra->cap) { \
                yyextra->tokens = sl_realloc(yyextra->vm->arena, yyextra->tokens, sizeof(sl_token_t) * (yyextra->cap *= 2)); \
            } \
            yyextra->tokens[yyextra->len] = (tok); \
            yyextra->tokens[yyextra->len].str = sl_make_string(yyextra->vm, (uint8_t*)yytext, yyleng); \
            yyextra->tokens[yyextra->len].line = yyextra->line; \
            yyextra->len++; \
        } while(0)
%}

%x SLASH STRING STRE COMMENT_ML_C COMMENT_LINE COMMENT_TAG NKW_ID REGEXP REGEXP_R

/* after each keyword, put '/{KW}' to look ahead for a non-identifier char */
NKW [^a-zA-Z_0-9]
SYM [a-zA-Z0-9_]+
IDT [a-zA-Z0-9_]*
ID  [a-z_]{IDT}
HEX [0-9a-fA-F]

%%

<INITIAL>"<%="      { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_ECHO_TAG));                 BEGIN(SLASH); }
<INITIAL>"<%!!"     { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_RAW_ECHO_TAG));             BEGIN(SLASH); }
<INITIAL>"<%#"      {                                                                 BEGIN(COMMENT_TAG); }
<INITIAL>"<%"       { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_TAG));                      BEGIN(SLASH); }

<INITIAL>\n         { sl_lex_append_to_raw(yyextra, yytext, 1); yyextra->line++; }
<INITIAL>.          { sl_lex_append_to_raw(yyextra, yytext, 1); }

<COMMENT_TAG>"%>"   {                                                                 BEGIN(INITIAL); }
<COMMENT_TAG>\n     { yyextra->line++; }
<COMMENT_TAG>.      { }

<COMMENT_LINE>.     { }
<COMMENT_LINE>\n    { yyextra->line++;                                                BEGIN(SLASH); }

<COMMENT_ML_C>"*/"  {                                                                 BEGIN(SLASH); }
<COMMENT_ML_C>\n    { yyextra->line++; }
<COMMENT_ML_C>.     { }

<STRING>"\\"        { BEGIN(STRE); }
<STRING>"\""        { BEGIN(SLASH); }
<STRING>\n          { sl_lex_append_byte_to_string(yyextra, yytext[0]); yyextra->line++; }
<STRING>.           { sl_lex_append_byte_to_string(yyextra, yytext[0]); }

<STRE>"n"           { sl_lex_append_byte_to_string(yyextra, '\n');                    BEGIN(STRING); }
<STRE>"t"           { sl_lex_append_byte_to_string(yyextra, '\t');                    BEGIN(STRING); }
<STRE>"r"           { sl_lex_append_byte_to_string(yyextra, '\r');                    BEGIN(STRING); }
<STRE>"e"           { sl_lex_append_byte_to_string(yyextra, '\033');                  BEGIN(STRING); }
<STRE>"x"{HEX}{2}   { sl_lex_append_hex_to_string(yyextra, yytext);                   BEGIN(STRING); }
<STRE>\n            { sl_lex_append_byte_to_string(yyextra, yytext[0]);               BEGIN(STRING); yyextra->line++; }
<STRE>.             { sl_lex_append_byte_to_string(yyextra, yytext[0]);               BEGIN(STRING); }

<SLASH>"\""         { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_STRING, "", 0)); BEGIN(STRING); }
<SLASH>"'"{SYM}     { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_STRING, yytext + 1, yyleng - 1)); }

<SLASH>"%r{"        { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_REGEXP, "", 0)); BEGIN(REGEXP); }
<REGEXP>"{"         { sl_lex_append_byte_to_string(yyextra, yytext[0]);               yy_push_state(REGEXP_R, yyscanner); }
<REGEXP>"\\"(.|\n)  { sl_lex_append_byte_to_string(yyextra, yytext[0]);
                      sl_lex_append_byte_to_string(yyextra, yytext[1]); }
<REGEXP>"}"[a-z]*   { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_REGEXP_OPTS, yytext + 1, yyleng - 1));
                                                                                      BEGIN(SLASH); }
<REGEXP>.|\n        { sl_lex_append_byte_to_string(yyextra, yytext[0]); }
<REGEXP_R>"{"       { sl_lex_append_byte_to_string(yyextra, yytext[0]);               yy_push_state(REGEXP_R, yyscanner); }
<REGEXP_R>"}"       { sl_lex_append_byte_to_string(yyextra, yytext[0]);               yy_pop_state(yyscanner); }
<REGEXP_R>"\\"(.|\n) {sl_lex_append_byte_to_string(yyextra, yytext[0]);
                      sl_lex_append_byte_to_string(yyextra, yytext[1]); }
<REGEXP_R>.|\n      { sl_lex_append_byte_to_string(yyextra, yytext[0]); }

<SLASH>"/*"         {                                                                 BEGIN(COMMENT_ML_C); }
<SLASH>"#"|"//"     {                                                                 BEGIN(COMMENT_LINE); }
<SLASH>"%>"         { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_TAG));                     BEGIN(INITIAL); }

<SLASH>"-"?[0-9]+"e"[+-]?[0-9]+                 { ADD_TOKEN(sl_make_float_token(yytext)); }
<SLASH>"-"?[0-9]+("."[0-9]+)("e"[+-]?[0-9]+)?   { ADD_TOKEN(sl_make_float_token(yytext)); }

<SLASH>"-"?[0-9]+           { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_INTEGER, yytext, yyleng)); }

<SLASH>"nil"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_NIL)); }
<SLASH>"true"/{NKW}     { ADD_TOKEN(sl_make_token(SL_TOK_TRUE)); }
<SLASH>"false"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_FALSE)); }
<SLASH>"self"/{NKW}     { ADD_TOKEN(sl_make_token(SL_TOK_SELF)); }
<SLASH>"class"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_CLASS)); }
<SLASH>"extends"/{NKW}  { ADD_TOKEN(sl_make_token(SL_TOK_EXTENDS)); }
<SLASH>"def"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_DEF)); }
<SLASH>"if"/{NKW}       { ADD_TOKEN(sl_make_token(SL_TOK_IF)); }
<SLASH>"elsif"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_ELSIF)); }
<SLASH>"else"/{NKW}     { ADD_TOKEN(sl_make_token(SL_TOK_ELSE)); }
<SLASH>"unless"/{NKW}   { ADD_TOKEN(sl_make_token(SL_TOK_UNLESS)); }
<SLASH>"for"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_FOR)); }
<SLASH>"in"/{NKW}       { ADD_TOKEN(sl_make_token(SL_TOK_IN)); }
<SLASH>"while"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_WHILE)); }
<SLASH>"until"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_UNTIL)); }
<SLASH>"and"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_LP_AND)); }
<SLASH>"or"/{NKW}       { ADD_TOKEN(sl_make_token(SL_TOK_LP_OR)); }
<SLASH>"not"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_LP_NOT)); }
<SLASH>"lambda"/{NKW}   { ADD_TOKEN(sl_make_token(SL_TOK_LAMBDA)); }
<SLASH>"try"/{NKW}      { ADD_TOKEN(sl_make_token(SL_TOK_TRY)); }
<SLASH>"catch"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_CATCH)); }
<SLASH>"return"/{NKW}   { ADD_TOKEN(sl_make_token(SL_TOK_RETURN)); }
<SLASH>"next"/{NKW}     { ADD_TOKEN(sl_make_token(SL_TOK_NEXT)); }
<SLASH>"last"/{NKW}     { ADD_TOKEN(sl_make_token(SL_TOK_LAST)); }
<SLASH>"throw"/{NKW}    { ADD_TOKEN(sl_make_token(SL_TOK_THROW)); }

<SLASH>"\\"             { ADD_TOKEN(sl_make_token(SL_TOK_LAMBDA)); }

<SLASH>[A-Z]{IDT}?  { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_CONSTANT, yytext, yyleng)); }
<SLASH>{ID}         { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_IDENTIFIER, yytext, yyleng)); }
<SLASH>"@"{ID}      { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_IVAR, yytext + 1, yyleng - 1)); }
<SLASH>"@@"{ID}     { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_CVAR, yytext + 2, yyleng - 2)); }

<SLASH>"("          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_PAREN)); }
<SLASH>")"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_PAREN)); }
<SLASH>"["          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_BRACKET)); }
<SLASH>"]"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_BRACKET)); }
<SLASH>"{"          { ADD_TOKEN(sl_make_token(SL_TOK_OPEN_BRACE)); }
<SLASH>"}"          { ADD_TOKEN(sl_make_token(SL_TOK_CLOSE_BRACE)); }
<SLASH>";"          { ADD_TOKEN(sl_make_token(SL_TOK_SEMICOLON)); }

<SLASH>"<<="        { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_SHIFT_LEFT)); }
<SLASH>">>="        { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_SHIFT_RIGHT)); }

<SLASH>"<<"         { ADD_TOKEN(sl_make_token(SL_TOK_SHIFT_LEFT)); }
<SLASH>">>"         { ADD_TOKEN(sl_make_token(SL_TOK_SHIFT_RIGHT)); }
<SLASH>","          { ADD_TOKEN(sl_make_token(SL_TOK_COMMA)); }
<SLASH>"=="         { ADD_TOKEN(sl_make_token(SL_TOK_DBL_EQUALS)); }
<SLASH>"!="         { ADD_TOKEN(sl_make_token(SL_TOK_NOT_EQUALS)); }
<SLASH>"=>"         { ADD_TOKEN(sl_make_token(SL_TOK_FAT_COMMA)); }
<SLASH>"="          { ADD_TOKEN(sl_make_token(SL_TOK_EQUALS)); }
<SLASH>"<=>"        { ADD_TOKEN(sl_make_token(SL_TOK_SPACESHIP)); }
<SLASH>"<="         { ADD_TOKEN(sl_make_token(SL_TOK_LTE)); }
<SLASH>"<"          { ADD_TOKEN(sl_make_token(SL_TOK_LT)); }
<SLASH>">="         { ADD_TOKEN(sl_make_token(SL_TOK_GTE)); }
<SLASH>">"          { ADD_TOKEN(sl_make_token(SL_TOK_GT)); }

<SLASH>"++"         { ADD_TOKEN(sl_make_token(SL_TOK_INCREMENT)); }
<SLASH>"--"         { ADD_TOKEN(sl_make_token(SL_TOK_DECREMENT)); }

<SLASH>"+="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_PLUS)); }
<SLASH>"-="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_MINUS)); }
<SLASH>"**="        { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_POW)); }
<SLASH>"*="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_TIMES)); }
<SLASH>"/="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_DIVIDE)); }
<SLASH>"%="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_MOD)); }

<SLASH>"+"          { ADD_TOKEN(sl_make_token(SL_TOK_PLUS)); }
<SLASH>"-"          { ADD_TOKEN(sl_make_token(SL_TOK_MINUS)); }
<SLASH>"**"         { ADD_TOKEN(sl_make_token(SL_TOK_POW)); }
<SLASH>"*"          { ADD_TOKEN(sl_make_token(SL_TOK_TIMES)); }
<SLASH>"/"          { ADD_TOKEN(sl_make_token(SL_TOK_DIVIDE)); }
<SLASH>"%"          { ADD_TOKEN(sl_make_token(SL_TOK_MOD)); }

<SLASH>"^="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_CARET)); }
<SLASH>"&="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_AMP)); }
<SLASH>"&&="        { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_AND)); }
<SLASH>"|="         { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_PIPE)); }
<SLASH>"||="        { ADD_TOKEN(sl_make_token(SL_TOK_ASSIGN_OR)); }

<SLASH>"^"          { ADD_TOKEN(sl_make_token(SL_TOK_CARET)); }
<SLASH>"~"          { ADD_TOKEN(sl_make_token(SL_TOK_TILDE)); }
<SLASH>"&"          { ADD_TOKEN(sl_make_token(SL_TOK_AMP)); }
<SLASH>"&&"         { ADD_TOKEN(sl_make_token(SL_TOK_AND)); }
<SLASH>"|"          { ADD_TOKEN(sl_make_token(SL_TOK_PIPE)); }
<SLASH>"||"         { ADD_TOKEN(sl_make_token(SL_TOK_OR)); }
<SLASH>"!"          { ADD_TOKEN(sl_make_token(SL_TOK_NOT)); }

<SLASH>"..."        { ADD_TOKEN(sl_make_token(SL_TOK_RANGE_EX)); }
<SLASH>".."         { ADD_TOKEN(sl_make_token(SL_TOK_RANGE)); }
<SLASH>"."/{ID}     { ADD_TOKEN(sl_make_token(SL_TOK_DOT));                                         BEGIN(NKW_ID); }
<NKW_ID>{ID}        { ADD_TOKEN(sl_make_string_token(yyextra, SL_TOK_IDENTIFIER, yytext, yyleng));  BEGIN(SLASH); }
<SLASH>"."          { ADD_TOKEN(sl_make_token(SL_TOK_DOT)); }
<SLASH>"::"         { ADD_TOKEN(sl_make_token(SL_TOK_PAAMAYIM_NEKUDOTAYIM)); }

<SLASH>[ \t\r]      { /* ignore */ }
<SLASH>\n           { yyextra->line++; }

<SLASH>.            { sl_lex_error(yyextra, yytext, yylineno); }

%%

sl_token_t*
sl_lex(sl_vm_t* vm, uint8_t* filename, uint8_t* buff, size_t len, size_t* token_count)
{
    yyscan_t yyscanner;
    YY_BUFFER_STATE buff_state = 0;
    sl_lex_state_t ls;
    sl_catch_frame_t frame;
    SLVAL err;
    ls.vm = vm;
    ls.cap = 8;
    ls.len = 0;
    ls.tokens = sl_alloc(vm->arena, sizeof(sl_token_t) * ls.cap);
    ls.filename = filename;
    ls.line = 1;
    
    ls.tokens[ls.len].line = 1;
    ls.tokens[ls.len++].type = SL_TOK_CLOSE_TAG;
    
    yylex_init_extra(&ls, &yyscanner);
    SL_TRY(frame, SL_UNWIND_ALL, {
        buff_state = yy_scan_bytes((char*)buff, len, yyscanner);
        yylex(yyscanner);
    }, err, {
        /* clean up to avoid memory leaks */
        yy_delete_buffer(buff_state, yyscanner);
        yylex_destroy(yyscanner);
        /* and rethrow */
        sl_rethrow(vm, &frame);
    });
    yy_delete_buffer(buff_state, yyscanner);
    yylex_destroy(yyscanner);
    
    ls.tokens[ls.len].line = ls.line;
    ls.tokens[ls.len++].type = SL_TOK_END;
    *token_count = ls.len;
    return ls.tokens;
}
