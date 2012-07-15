#include <slash.h>
#include <stdio.h>
#include <stdlib.h>

static void lex(sl_vm_t* vm, void* state)
{    
    size_t token_count;
    (void)state;
    sl_lex(vm, (uint8_t*)"-", (uint8_t*)"Justice - †", 13, &token_count);
}

static void syntax_error(sl_vm_t* vm, void* state, SLVAL error)
{
    SLVAL error_str = sl_to_s(vm, error);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(error_str);
    (void)state;
    fwrite(str->buff, str->buff_len, 1, stderr);
    exit(1);
}

int main()
{
    sl_vm_t* vm;
    sl_static_init();
    vm = sl_init();
    sl_try(vm, lex, syntax_error, NULL);
    return 0;
}
