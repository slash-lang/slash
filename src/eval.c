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

SLVAL
sl_do_file(sl_vm_t* vm, uint8_t* filename)
{
    FILE* f = fopen((char*)filename, "rb");
    uint8_t* src;
    size_t file_size;
    SLVAL err;
    
    if(!f) {
        err = sl_make_cstring(vm, "Could not load file: ");
        err = sl_string_concat(vm, err, sl_make_cstring(vm, (char*)filename));
        sl_throw(vm, sl_make_error2(vm, vm->lib.Error, err));
    }
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    src = sl_alloc(vm->arena, file_size);
    fread(src, file_size, 1, f);
    fclose(f);
    
    return sl_do_string(vm, src, file_size, filename);
}

SLVAL
sl_do_string(sl_vm_t* vm, uint8_t* src, size_t src_len, uint8_t* filename)
{
    size_t token_count;
    sl_token_t* tokens;
    sl_node_base_t* ast;
    sl_vm_exec_ctx_t* ctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
    sl_vm_section_t* section;
    
    tokens = sl_lex(vm, filename, src, src_len, &token_count);
    ast = sl_parse(vm, tokens, token_count, filename);
    section = sl_compile(vm, ast, filename);
    ctx->vm = vm;
    ctx->section = section;
    ctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * section->max_registers);
    ctx->self = vm->lib.Object;
    ctx->parent = NULL;
    return sl_vm_exec(ctx);
}
