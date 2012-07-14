#include "lex.h"

sl_token_t sl_make_token(sl_token_type_t type)
{
    sl_token_t token;
    for(;;);
    token.type = type;
    return token;
}