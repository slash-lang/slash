#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <slash/vm.h>
#include <slash/value.h>
#include <slash/dispatch.h>
#include <slash/method.h>
#include <slash/string.h>
#include <slash/object.h>

int
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id)
{
    return sl_responds_to2(vm, object, sl_intern(vm, id));
}

int
sl_responds_to2(sl_vm_t* vm, SLVAL object, SLID id)
{
    return sl_lookup_method(vm, object, id) != NULL;
}

static SLVAL
call_c_func(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    if(method->arity < 0) {
        return method->as.c.func(vm, recv, argc, argv);
    } else {
        switch(method->arity) {
            case 0: return method->as.c.func(vm, recv);
            case 1: return method->as.c.func(vm, recv, argv[0]);
            case 2: return method->as.c.func(vm, recv, argv[0], argv[1]);
            case 3: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2]);
            case 4: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2], argv[3]);
            default:
                sl_throw_message(vm, "Too many arguments for C function");
        }
    }
    return vm->lib.nil; /* never reached */
}

static SLVAL
call_c_func_guard(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    sl_vm_frame_t frame;
    frame.prev = vm->call_stack;
    frame.frame_type = SL_VM_FRAME_C;
    frame.as.call_frame.method = method->name;
    vm->call_stack = &frame;
    
    SLVAL retn = call_c_func(vm, recv, method, argc, argv);
    
    vm->call_stack = frame.prev;
    return retn;
}

SLVAL
sl_apply_method(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    char errstr[1024];
    sl_vm_exec_ctx_t stack_ctx;
    sl_vm_exec_ctx_t* ctx;
    size_t i;
    SLVAL arg;
    if((void*)&arg < vm->stack_limit) {
        /* we're about to blow the stack */
        sl_throw(vm, vm->lib.StackOverflowError_instance);
    }
    if(method->arity < 0) {
        if(sl_unlikely((size_t)(-method->arity - 1) > argc)) {
            sprintf(errstr, "Too few arguments. Expected %d, received %lu.", (-method->arity - 1), (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    } else {
        if(sl_unlikely((size_t)method->arity > argc)) {
            sprintf(errstr, "Too few arguments. Expected %d, received %lu.", method->arity, (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
        if(sl_unlikely((size_t)method->arity < argc)) {
            sprintf(errstr, "Too many arguments. Expected %d, received %lu.", method->arity, (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    }
    if(method->is_c_func) {
        return call_c_func_guard(vm, recv, method, argc, argv);
    } else {
        if(sl_likely(method->as.sl.section->can_stack_alloc_frame)) {
            ctx = &stack_ctx;
            memset(ctx, 0, sizeof(*ctx));
        } else {
            ctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
        }
        ctx->vm = vm;
        ctx->section = method->as.sl.section;
        if(method->as.sl.section->can_stack_alloc_frame) {
            ctx->registers = alloca(sizeof(SLVAL) * ctx->section->max_registers);
        } else {
            ctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * ctx->section->max_registers);
        }
        ctx->self = recv;
        ctx->parent = method->as.sl.parent_ctx;
        
        for(i = 0; i < ctx->section->arg_registers; i++) {
            ctx->registers[i + 1] = argv[i];
        }
        
        if(argc > ctx->section->arg_registers) {
            argc = ctx->section->arg_registers;
        }
        if(ctx->section->opt_skip) {
            return sl_vm_exec(ctx, ctx->section->opt_skip[argc - ctx->section->req_registers]);
        } else {
            return sl_vm_exec(ctx, 0);
        }
    }
    return vm->lib.nil;
}

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...)
{
    SLVAL* argv = alloca(argc * sizeof(SLVAL));
    va_list va;
    size_t i;
    va_start(va, argc);
    for(i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, sl_intern(vm, id), argc, argv);
}

SLVAL
sl_send_id(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, ...)
{
    SLVAL argv[argc];
    va_list va;
    size_t i;
    va_start(va, argc);
    for(i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, id, argc, argv);
}

static SLVAL
sl_send_missing(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, SLVAL* argv)
{
    /* look for method_missing method */
    SLVAL argv2[argc + 1];
    memcpy(&argv2[1], argv, sizeof(SLVAL) * argc);
    argv2[0] = sl_id_to_string(vm, id);
    
    sl_method_t* method = sl_lookup_method(vm, recv, vm->id.method_missing);
    if(method) {
        return sl_apply_method(vm, recv, method, argc + 1, argv2);
    }
    
    /* nope */
    
    SLVAL error = sl_make_cstring(vm, "Undefined method '");
    error = sl_string_concat(vm, error, sl_id_to_string(vm, id));
    error = sl_string_concat(vm, error, sl_make_cstring(vm, "' on "));
    error = sl_string_concat(vm, error, sl_object_inspect(vm, recv));
    sl_throw(vm, sl_make_error2(vm, vm->lib.NoMethodError, error));
    return vm->lib.nil; /* shutup gcc */
}

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, SLVAL* argv)
{
    sl_method_t* method = sl_lookup_method(vm, recv, id);
    if(sl_likely(method != NULL)) {
        return sl_apply_method(vm, recv, method, argc, argv);
    }
    return sl_send_missing(vm, recv, id, argc, argv);   
}

static sl_method_t*
lookup_method_rec(sl_vm_t* vm, SLVAL klass, SLID id)
{
    sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
    if(klassp->base.primitive_type == SL_T_NIL) {
        return NULL;
    }

    sl_method_t* method;
    sl_cached_method_entry_t* cme = NULL;
    if(sl_st_lookup(klassp->instance_methods, (sl_st_data_t)id.id, (sl_st_data_t*)&method)) {
        if(method->base.primitive_type == SL_T_CACHED_METHOD_ENTRY) {
            cme = (sl_cached_method_entry_t*)method;
            // TODO - improve cache invalidation. this is too coarse
            if(cme->state == vm->state_method) {
                return cme->method;
            }
        } else {
            return method;
        }
    }

    method = lookup_method_rec(vm, klassp->super, id);
    if(!cme) {
        cme = sl_alloc(vm->arena, sizeof(*cme));
    }
    cme->primitive_type = SL_T_CACHED_METHOD_ENTRY;
    cme->state = vm->state_method;
    cme->method = method;
    sl_st_insert(klassp->instance_methods, (sl_st_data_t)id.id, (sl_st_data_t)cme);
    return method;
}

sl_method_t*
sl_lookup_method(sl_vm_t* vm, SLVAL recv, SLID id)
{
    SLVAL klass = SL_IS_INT(recv) ? vm->lib.Int : sl_get_ptr(recv)->klass;
    return lookup_method_rec(vm, klass, id);
}

static SLVAL
sl_imc_cached_call(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL recv, SLVAL* argv)
{
    return sl_apply_method(vm, recv, imc->method, imc->argc, argv);
}

SLVAL
sl_imc_setup_call(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL recv, SLVAL* argv)
{
    SLVAL klass = SL_IS_INT(recv) ? vm->lib.Int : sl_get_ptr(recv)->klass;
    sl_method_t* method = lookup_method_rec(vm, klass, imc->id);
    if(method) {
        imc->state = vm->state_method;
        imc->klass = klass;
        imc->method = method;
        imc->call = sl_imc_cached_call;
        return sl_imc_cached_call(vm, imc, recv, argv);
    } else {
        return sl_send_missing(vm, recv, imc->id, imc->argc, argv);
    }
}
