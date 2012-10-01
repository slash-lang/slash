#include <slash/vm.h>
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
#define NEXT_REG() (ctx->registers[NEXT_UINT()])
#define NEXT_STR() (NEXT().str)
#define NEXT_SECTION() (NEXT().section)

#define V_TRUE (vm->lib._true)
#define V_FALSE (vm->lib._false)
#define V_NIL (vm->lib.nil)

#define INSTRUCTION(opcode, code) \
                case opcode: { \
                    code \
                } break;

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx)
{
    size_t ip = 0;
    int line = 0;
    sl_vm_t* vm = ctx->vm;
    while(1) {
        switch(NEXT().opcode) {
            #include "vm_defn.inc"
        }
    }
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
