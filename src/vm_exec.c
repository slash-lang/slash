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
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLVAL name, SLVAL extends, sl_vm_section_t* section);

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLVAL name, sl_vm_section_t* section);

static SLVAL
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLVAL name, sl_vm_section_t* section);

#define NEXT() (ctx->section->insns[ip++])

#define NEXT_IMM() (NEXT().imm)
#define NEXT_UINT() (NEXT().uint)
#define NEXT_PTR() ((void*)NEXT().uint)
#define NEXT_REG() (ctx->registers[NEXT_UINT()])
#define NEXT_STR() (NEXT().str)
#define NEXT_SECTION() (NEXT().section)

#define V_TRUE (vm->lib._true)
#define V_FALSE (vm->lib._false)
#define V_NIL (vm->lib.nil)

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx)
{
    size_t ip = 0;
    volatile int line = 0;
    sl_vm_t* vm = ctx->vm;
    sl_catch_frame_t frame;
    
    #if 0
        void* jump_table[] = {
            #define INSTRUCTION(opcode, code) [opcode] = &&vm_##opcode,
            #include "vm_defn.inc"
            NULL
        };
        
        goto *jump_table[NEXT().opcode];
        
        #undef INSTRUCTION
        #define INSTRUCTION(op_name, code) \
            vm_##op_name: { \
                code; \
            } \
            goto *jump_table[NEXT().opcode];
        
        #include "vm_defn.inc"
    #else
        frame.prev = vm->catch_stack;
        frame.value = vm->lib.nil;
        vm->catch_stack = &frame;
        if(!setjmp(frame.env)) {
            while(1) {
                /* for non-gcc compilers, fall back to switch in a loop */
                #define INSTRUCTION(opcode, code) \
                                case opcode: { \
                                    code; \
                                } break;
                switch(NEXT().opcode) {
                    #include "vm_defn.inc"
                }
            }
        } else {
            vm->catch_stack = frame.prev;
            if(frame.type & SL_UNWIND_EXCEPTION) {
                sl_error_add_frame(vm, frame.value, ctx->self, V_NIL, V_NIL, sl_make_int(vm, line));
            }
            sl_rethrow(vm, &frame);
            /* shut up gcc */
            return V_NIL;
        }
    #endif
}

/* helper functions */

static SLVAL
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLVAL name, SLVAL extends, sl_vm_section_t* section)
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
    sl_vm_exec(subctx);
    return klass;
}

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLVAL name, sl_vm_section_t* section)
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
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLVAL name, sl_vm_section_t* section)
{
    SLVAL klass = on, method;
    if(!sl_is_a(ctx->vm, klass, ctx->vm->lib.Class)) {
        klass = sl_class_of(ctx->vm, klass);
    }
    method = sl_make_method(ctx->vm, klass, name, section, ctx);
    sl_define_singleton_method3(ctx->vm, on, name, method);
    return method;
}
