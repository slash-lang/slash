#include "lib/lambda.h"
#include "slash.h"
#include <stdio.h>

typedef struct {
    sl_object_t base;
    sl_node_lambda_t* node;
    sl_eval_ctx_t* ctx;
}
sl_lambda_t;

sl_lambda_t*
get_lambda(sl_vm_t* vm, SLVAL obj)
{
    return (sl_lambda_t*)sl_get_ptr(sl_expect(vm, obj, vm->lib.Lambda));
}

SLVAL
sl_make_lambda(sl_node_lambda_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL lambda = sl_allocate(ctx->vm, ctx->vm->lib.Lambda);
    sl_lambda_t* lp = get_lambda(ctx->vm, lambda);
    lp->node = node;
    lp->ctx = ctx;
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
    sl_eval_ctx_t* subctx;
    if(argc < lambda->node->arg_count) {
        err = sl_make_cstring(vm, "Too few arguments in lambda call. Expected ");
        sprintf(buff, "%d", (int)lambda->node->arg_count);
        err = sl_string_concat(vm, err, sl_make_cstring(vm, buff));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, ", received "));
        sprintf(buff, "%d", (int)argc);
        err = sl_string_concat(vm, err, sl_make_cstring(vm, buff));
        sl_throw(vm, sl_make_error2(vm, vm->lib.ArgumentError, err));
    }
    subctx = sl_close_eval_ctx(vm, lambda->ctx);
    for(i = 0; i < lambda->node->arg_count; i++) {
        st_insert(subctx->vars, (st_data_t)lambda->node->args[i], (st_data_t)sl_get_ptr(argv[i]));
    }
    return lambda->node->body->eval(lambda->node->body, subctx);
}

void
sl_init_lambda(sl_vm_t* vm)
{
    vm->lib.Lambda = sl_define_class(vm, "Lambda", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Lambda, allocate_lambda);
    sl_define_method(vm, vm->lib.Lambda, "call", -1, sl_lambda_call);
}
