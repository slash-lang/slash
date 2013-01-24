#include <slash/vm.h>
#include <slash/error.h>
#include <slash/value.h>
#include <slash/string.h>
#include <slash/object.h>
#include <slash/class.h>
#include <slash/method.h>
#include <slash/lib/response.h>
#include <slash/lib/array.h>
#include <slash/lib/dict.h>
#include <slash/lib/range.h>
#include <slash/lib/lambda.h>
#include <stdio.h>

static SLVAL
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL extends, sl_vm_section_t* section);

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLID name, sl_vm_section_t* section);

static SLVAL
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLID name, sl_vm_section_t* section);

#define NEXT() (ctx->section->insns[ip++])

#define NEXT_IMM() (NEXT().imm)
#define NEXT_UINT() (NEXT().uint)
#define NEXT_PTR() ((void*)NEXT().uint)
#define NEXT_IC() (NEXT().ic)
#define NEXT_ID() (NEXT().id)
#define NEXT_REG() (ctx->registers[NEXT_UINT()])
#define NEXT_STR() (NEXT().str)
#define NEXT_SECTION() (NEXT().section)

#define V_TRUE (vm->lib._true)
#define V_FALSE (vm->lib._false)
#define V_NIL (vm->lib.nil)

#define INSTRUCTION(opcode, code) \
    case opcode: { \
        code; \
    } break;

typedef struct sl_vm_exception_handler {
    struct sl_vm_exception_handler* prev;
    size_t catch_ip;
}
sl_vm_exception_handler_t;

static SLVAL
sl_vm_exec_no_exceptions(sl_vm_exec_ctx_t* ctx, volatile size_t ip)
{
    volatile int line = 0;
    sl_vm_t* vm = ctx->vm;
    sl_vm_section_t* section = ctx->section;

    sl_vm_frame_t call_frame;
    call_frame.prev = vm->call_stack;
    call_frame.frame_type = SL_VM_FRAME_SLASH;
    call_frame.as.call_frame.method = section->name;
    call_frame.as.call_frame.filename = (char*)section->filename;
    call_frame.as.call_frame.line = (int*)&line;
    vm->call_stack = &call_frame;

    while(1) {
        switch(NEXT().opcode) {
            #include "vm_defn.inc"
            
            default:
                sl_throw_message(vm, "BUG: Unknown opcode in VM"); /* never reached */
        }
    }
}

static SLVAL
sl_vm_exec_with_exceptions(sl_vm_exec_ctx_t* ctx, volatile size_t ip)
{
    volatile int line = 0;
    sl_vm_t* vm = ctx->vm;
    sl_vm_exception_handler_t* volatile exception_handler = NULL;
    sl_vm_section_t* section = ctx->section;

    sl_vm_frame_t call_frame;
    call_frame.prev = vm->call_stack;
    call_frame.frame_type = SL_VM_FRAME_SLASH;
    call_frame.as.call_frame.method = section->name;
    call_frame.as.call_frame.filename = (char*)section->filename;
    call_frame.as.call_frame.line = (int*)&line;
    vm->call_stack = &call_frame;

    sl_vm_frame_t catch_frame; /* TODO clean this shit up */
    catch_frame.frame_type = SL_VM_FRAME_HANDLER;
    catch_frame.as.handler_frame.value = vm->lib.nil;
reset_exception_handler:
    catch_frame.prev = vm->call_stack;
    vm->call_stack = &catch_frame;
    
    if(sl_setjmp(catch_frame.as.handler_frame.env)) {
        vm->call_stack = catch_frame.prev;
        if(exception_handler && (catch_frame.as.handler_frame.unwind_type & SL_UNWIND_EXCEPTION)) {
            ip = exception_handler->catch_ip;
            goto reset_exception_handler;
        } else {
            vm->call_stack = call_frame.prev;
            sl_unwind(vm, catch_frame.as.handler_frame.value, catch_frame.as.handler_frame.unwind_type);
        }
    }

    while(1) {
        switch(NEXT().opcode) {
            #define WITH_EXCEPTION_OPCODES
            #include "vm_defn.inc"
            
            default:
                sl_throw_message(vm, "BUG: Unknown opcode in VM"); /* never reached */
        }
    }
}

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx, volatile size_t ip)
{
    if(ctx->section->has_try_catch) {
        return sl_vm_exec_with_exceptions(ctx, ip);
    } else {
        return sl_vm_exec_no_exceptions(ctx, ip);
    }
}

/* helper functions */

static SLVAL
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL extends, sl_vm_section_t* section)
{
    SLVAL klass, in;
    sl_vm_exec_ctx_t* subctx;
    if(sl_class_has_const2(ctx->vm, ctx->self, name)) {
        /* @TODO: verify same superclass */
        klass = sl_class_get_const2(ctx->vm, ctx->self, name);
    } else {
        in = ctx->self;
        if(!sl_is_a(ctx->vm, in, ctx->vm->lib.Class)) {
            in = sl_class_of(ctx->vm, in);
        }
        klass = sl_define_class3(ctx->vm, name, extends, in);
    }
    subctx = sl_alloc(ctx->vm->arena, sizeof(sl_vm_exec_ctx_t));
    subctx->vm = ctx->vm;
    subctx->section = section;
    subctx->registers = sl_alloc(ctx->vm->arena, sizeof(SLVAL) * section->max_registers);
    subctx->self = klass;
    subctx->parent = ctx;
    sl_vm_exec(subctx, 0);
    return klass;
}

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLID name, sl_vm_section_t* section)
{
    SLVAL on = ctx->self, method;
    if(!sl_is_a(ctx->vm, on, ctx->vm->lib.Class)) {
        on = sl_class_of(ctx->vm, on);
    }
    method = sl_make_method(ctx->vm, on, name, section, ctx);
    sl_define_method3(ctx->vm, on, name, method);
    return method;
}

static SLVAL
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLID name, sl_vm_section_t* section)
{
    SLVAL klass = on, method;
    if(!sl_is_a(ctx->vm, klass, ctx->vm->lib.Class)) {
        klass = sl_class_of(ctx->vm, klass);
    }
    method = sl_make_method(ctx->vm, klass, name, section, ctx);
    sl_define_singleton_method3(ctx->vm, on, name, method);
    return method;
}
