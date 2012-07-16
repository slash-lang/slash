#include "slash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    (void)vm;
    fwrite(buff, len, 1, stdout);
}

static void run(sl_vm_t* vm, void* state)
{    
    size_t token_count;
    sl_token_t* tokens;
    sl_node_base_t* ast;
    char* src = "one plus two is: <%= 1 + 2 %>";
    uint8_t* filename = (uint8_t*)"-";
    sl_eval_ctx_t ctx;
    (void)state;
    tokens = sl_lex(vm, filename, (uint8_t*)src, strlen(src), &token_count);
    ast = sl_parse(vm, tokens, token_count, filename);
    ctx.vm = vm;
    ctx.self = vm->lib.Object;
    ast->eval(ast, &ctx);
}

static void on_error(sl_vm_t* vm, void* state, SLVAL error)
{
    SLVAL error_str = sl_to_s(vm, error);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(error_str);
    (void)state;
    fwrite(str->buff, str->buff_len, 1, stderr);
    puts("\n");
    exit(1);
}

int main()
{
    sl_vm_t* vm;
    sl_static_init();
    vm = sl_init();
    vm->output = output;
    sl_try(vm, run, on_error, NULL);
    return 0;
}
