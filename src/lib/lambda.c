#include <slash.h>
#include <stdio.h>

typedef struct {
    sl_object_t base;
    sl_vm_section_t* section;
    sl_vm_exec_ctx_t* parent_ctx;
}
sl_lambda_t;

sl_lambda_t*
get_lambda(sl_vm_t* vm, SLVAL obj)
{
    return (sl_lambda_t*)sl_get_ptr(sl_expect(vm, obj, vm->lib.Lambda));
}
SLVAL
sl_make_lambda(sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx)
{
    SLVAL lambda = sl_allocate(parent_ctx->vm, parent_ctx->vm->lib.Lambda);
    sl_lambda_t* lp = get_lambda(parent_ctx->vm, lambda);
    lp->section = section;
    lp->parent_ctx = parent_ctx;
    return lambda;
}

static sl_object_t*
allocate_lambda(sl_vm_t* vm)
{
    return sl_alloc(vm->arena, sizeof(sl_lambda_t));
}

static SLVAL
sl_lambda_call(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_lambda_t* lambda = get_lambda(vm, self);
    SLVAL err;
    char buff[128];
    size_t i;
    sl_vm_exec_ctx_t* subctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
    if(argc < lambda->section->arg_registers) {
        err = sl_make_cstring(vm, "Too few arguments in lambda call. Expected ");
        sprintf(buff, "%d", (int)lambda->section->arg_registers);
        err = sl_string_concat(vm, err, sl_make_cstring(vm, buff));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, ", received "));
        sprintf(buff, "%d", (int)argc);
        err = sl_string_concat(vm, err, sl_make_cstring(vm, buff));
        sl_throw(vm, sl_make_error2(vm, vm->lib.ArgumentError, err));
    }
    subctx->vm = vm;
    subctx->section = lambda->section;
    subctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * lambda->section->max_registers);
    for(i = 0; i < lambda->section->max_registers; i++) {
        subctx->registers[i] = vm->lib.nil;
    }
    subctx->self = lambda->parent_ctx->self;
    subctx->parent = lambda->parent_ctx;
    for(i = 0; i < lambda->section->arg_registers; i++) {
        subctx->registers[i + 1] = argv[i];
    }
    return sl_vm_exec(subctx);
}

void
sl_init_lambda(sl_vm_t* vm)
{
    vm->lib.Lambda = sl_define_class(vm, "Lambda", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Lambda, allocate_lambda);
    sl_define_method(vm, vm->lib.Lambda, "call", -1, sl_lambda_call);
}
