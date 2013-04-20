#include <slash/eval.h>
#include <slash/object.h>
#include <slash/string.h>
#include <slash/class.h>
#include <slash/lex.h>
#include <slash/parse.h>
#include <slash/method.h>
#include <slash/lib/array.h>
#include <slash/lib/response.h>
#include <slash/lib/range.h>
#include <slash/platform.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

SLVAL
sl_do_file(sl_vm_t* vm, char* filename)
{
    filename = sl_realpath(vm, filename);
    FILE* f = fopen(filename, "rb");
    uint8_t* src;
    size_t file_size;
    
    if(!f) {
        sl_error(vm, vm->lib.Error, "Could not load %Qs - %s", filename, strerror(errno));
    }
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    src = sl_alloc(vm->arena, file_size);
    if(file_size && !fread(src, file_size, 1, f)) {
        fclose(f);
        sl_error(vm, vm->lib.Error, "Could not load %Qs - %s", filename, strerror(errno));
    }
    fclose(f);
    
    return sl_do_string(vm, src, file_size, filename, 0);
}

SLVAL
sl_do_string2(sl_vm_t* vm, uint8_t* src, size_t src_len, char* filename, int start_in_slash, SLVAL self)
{
    sl_vm_exec_ctx_t* ctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
    size_t token_count;
    sl_token_t* tokens = sl_lex(vm, (uint8_t*)filename, src, src_len, &token_count, start_in_slash);
    sl_node_base_t* ast = sl_parse(vm, tokens, token_count, (uint8_t*)filename);
    sl_vm_section_t* section = sl_compile(vm, ast, (uint8_t*)filename);
    ctx->vm = vm;
    ctx->section = section;
    ctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * section->max_registers);
    ctx->self = self;
    ctx->parent = NULL;
    return sl_vm_exec(ctx, 0);
}

SLVAL
sl_do_string(sl_vm_t* vm, uint8_t* src, size_t src_len, char* filename, int start_in_slash)
{
    return sl_do_string2(vm, src, src_len, filename, start_in_slash, vm->lib.Object);
}

static SLVAL
sl_eval(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_string_t* src = sl_get_string(vm, argv[0]);
    char* filename = "(eval)";
    if(argc > 1) {
        filename = sl_to_cstr(vm, argv[1]);
    }
    return sl_do_string2(vm, src->buff, src->buff_len, filename, 1, self);
}

void
sl_init_eval(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "eval", -2, sl_eval);
}
