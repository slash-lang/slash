#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <slash/vm.h>
#include <slash/value.h>
#include <slash/dispatch.h>
#include <slash/method.h>
#include <slash/string.h>
#include <slash/object.h>
#include <slash/lib/array.h>

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
    frame.as.c_call_frame.method = method;
    vm->call_stack = &frame;

    SLVAL retn = call_c_func(vm, recv, method, argc, argv);

    vm->call_stack = frame.prev;
    return retn;
}

static SLVAL
call_sl_func(sl_vm_t* vm, SLVAL recv, sl_method_t* method, int argc, SLVAL* argv)
{
    sl_vm_exec_ctx_t stack_ctx;
    sl_vm_exec_ctx_t* ctx;
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

    memcpy(ctx->registers + 1, argv, sizeof(SLVAL) * ctx->section->arg_registers);

    if(ctx->section->has_extra_rest_arg) {
        if(argc > ctx->section->arg_registers) {
            size_t extra_len = argc - ctx->section->arg_registers;
            ctx->registers[ctx->section->arg_registers + 1] = sl_make_array(vm, extra_len, argv + ctx->section->arg_registers);
        } else {
            ctx->registers[ctx->section->arg_registers + 1] = sl_make_array(vm, 0, NULL);
        }
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

SLVAL
sl_apply_method(sl_vm_t* vm, SLVAL recv, sl_method_t* method, int argc, SLVAL* argv)
{
    SLVAL arg;
    if((void*)&arg < vm->stack_limit) {
        /* we're about to blow the stack */
        sl_throw(vm, vm->lib.StackOverflowError_instance);
    }
    if(method->arity < 0) {
        if(sl_unlikely(-method->arity - 1 > argc)) {
            sl_error(vm, vm->lib.ArgumentError, "Too few arguments. Expected %d, received %d.", (-method->arity - 1), argc);
        }
    } else {
        if(sl_unlikely(method->arity > argc)) {
            sl_error(vm, vm->lib.ArgumentError, "Too few arguments. Expected %d, received %d.", method->arity, argc);
        }
        if(sl_unlikely(method->arity < argc)) {
            sl_error(vm, vm->lib.ArgumentError, "Too many arguments. Expected %d, received %d.", method->arity, argc);
        }
    }
    if(method->is_c_func) {
        return call_c_func_guard(vm, recv, method, argc, argv);
    } else {
        return call_sl_func(vm, recv, method, argc, argv);
    }
}

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, int argc, ...)
{
    SLVAL* argv = alloca(argc * sizeof(SLVAL));
    va_list va;
    va_start(va, argc);
    for(int i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, sl_intern(vm, id), argc, argv);
}

SLVAL
sl_send_id(sl_vm_t* vm, SLVAL recv, SLID id, int argc, ...)
{
    SLVAL argv[argc];
    va_list va;
    va_start(va, argc);
    for(int i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, id, argc, argv);
}

static SLVAL
sl_send_missing(sl_vm_t* vm, SLVAL recv, SLID id, int argc, SLVAL* argv)
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

    sl_error(vm, vm->lib.NoMethodError, "Undefined method %QI on %X", id, recv);
    return vm->lib.nil; /* shutup gcc */
}

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLID id, int argc, SLVAL* argv)
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
    if(sl_st_lookup(klassp->instance_methods, (sl_st_data_t)id.id, (sl_st_data_t*)&method)) {
        return method;
    }

    return lookup_method_rec(vm, klassp->super, id);
}

sl_method_t*
sl_lookup_method(sl_vm_t* vm, SLVAL recv, SLID id)
{
    SLVAL klass = SL_IS_INT(recv) ? vm->lib.Int : sl_get_ptr(recv)->klass;
    return lookup_method_rec(vm, klass, id);
}

#define C_FRAME_BEGIN \
    if(sl_unlikely((void*)&vm < vm->stack_limit)) {         \
        /* we're about to blow the stack */                 \
        sl_throw(vm, vm->lib.StackOverflowError_instance);  \
    }                                                       \
    sl_vm_frame_t frame;                                    \
    frame.prev = vm->call_stack;                            \
    frame.frame_type = SL_VM_FRAME_C;                       \
    frame.as.c_call_frame.method = imc->method;             \
    vm->call_stack = &frame;

#define C_FRAME_END \
    vm->call_stack = frame.prev

static SLVAL
dispatch_c_call_variadic(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL recv, SLVAL* argv)
{
    C_FRAME_BEGIN;
    SLVAL ret = imc->method->as.c.func(vm, recv, imc->argc, argv);
    C_FRAME_END;
    return ret;
}

#define C_FUNC_FIXED_ARG_DISPATCHER(name, args) \
    static SLVAL \
    name(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL recv, SLVAL* argv) \
    { \
        C_FRAME_BEGIN; \
        SLVAL ret = imc->method->as.c.func args; \
        C_FRAME_END; \
        return ret; \
        (void)argv; \
    }

C_FUNC_FIXED_ARG_DISPATCHER(dispatch_c_call_0, (vm, recv))
C_FUNC_FIXED_ARG_DISPATCHER(dispatch_c_call_1, (vm, recv, argv[0]))
C_FUNC_FIXED_ARG_DISPATCHER(dispatch_c_call_2, (vm, recv, argv[0], argv[1]))
C_FUNC_FIXED_ARG_DISPATCHER(dispatch_c_call_3, (vm, recv, argv[0], argv[1], argv[2]))
C_FUNC_FIXED_ARG_DISPATCHER(dispatch_c_call_4, (vm, recv, argv[0], argv[1], argv[2], argv[3]))

static SLVAL
dispatch_slash_call_stack_regs(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL recv, SLVAL* argv)
{
    SLVAL regs[imc->method->as.sl.section->max_registers];
    sl_vm_exec_ctx_t ctx;
    ctx.vm = vm;
    ctx.section = imc->method->as.sl.section;
    ctx.registers = regs;
    ctx.self = recv;
    ctx.parent = imc->method->as.sl.parent_ctx;
    memcpy(ctx.registers + 1, argv, sizeof(SLVAL) * ctx.section->arg_registers);
    return sl_vm_exec(&ctx, 0);
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
        if(method->is_c_func) {
            if(method->arity < 0) {
                if(sl_likely(-imc->argc - 1 <= method->arity)) {
                    imc->call = dispatch_c_call_variadic;
                } else {
                    imc->call = sl_imc_cached_call; // cop out
                }
            } else {
                if(sl_likely(imc->argc == method->arity)) {
                    switch(method->arity) {
                        case 0: imc->call = dispatch_c_call_0; break;
                        case 1: imc->call = dispatch_c_call_1; break;
                        case 2: imc->call = dispatch_c_call_2; break;
                        case 3: imc->call = dispatch_c_call_3; break;
                        case 4: imc->call = dispatch_c_call_4; break;
                        default:
                            sl_throw_message(vm, "Too many arguments for C function");
                    }
                } else {
                    imc->call = sl_imc_cached_call; // cop out
                }
            }
        } else {
            if(method->arity >= 0 && imc->argc >= method->arity && method->as.sl.section->can_stack_alloc_frame && !method->as.sl.section->opt_skip) {
                imc->call = dispatch_slash_call_stack_regs;
            } else {
                imc->call = sl_imc_cached_call; // cop out
            }
        }
        return imc->call(vm, imc, recv, argv);
    } else {
        return sl_send_missing(vm, recv, imc->id, imc->argc, argv);
    }
}
