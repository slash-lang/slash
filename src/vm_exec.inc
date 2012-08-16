#include "vm.h"
#include "value.h"
#include "string.h"
#include "object.h"
#include "class.h"
#include "lib/response.h"
#include "lib/array.h"
#include "lib/dict.h"
#include "lib/range.h"
#include "lib/lambda.h"
#include <stdio.h>

#define VM_BEGIN \
    SLVAL \
    sl_vm_exec(sl_vm_exec_ctx_t* ctx) \
    { \
        size_t ip = 0; \
        sl_vm_t* vm = ctx->vm; \
        SLVAL tmp, tmp2, *tmp_p; \
        sl_string_t* tmp_str; \
        size_t tmp_uint; \
        sl_vm_exec_ctx_t* tmp_ctx; \
        sl_vm_section_t* tmp_section; \
        while(1) { \
            switch(NEXT().opcode) {

#define NEXT() (ctx->section->insns[ip++])

#define NEXT_IMM() (NEXT().imm)
#define NEXT_UINT() (NEXT().uint)
#define NEXT_REG() (ctx->registers[NEXT_UINT()])
#define NEXT_STR() (NEXT().str)
#define NEXT_SECTION() (NEXT().section)

#define V_TRUE (vm->lib._true)
#define V_FALSE (vm->lib._false)
#define V_NIL (vm->lib.nil)

#ifdef SL_DEBUG
    #define TRACE(fmt, ...) fprintf(stderr, "%04lu " fmt "\n", ip, __VA_ARGS__)
    #define DISPATCH_TRACE(x) TRACE("%s", x)
#else
    #define TRACE(...) 
    #define DISPATCH_TRACE(x)
#endif

#define INSTRUCTION(opcode, code) \
                case opcode: \
                    /*/DISPATCH_TRACE(#opcode);/**/ \
                    code \
                break;
    
#define VM_END \
            } \
        } \
    }

/* helper functions */

static SLVAL
sl_vm_define_class(sl_vm_exec_ctx_t* ctx, SLVAL name, SLVAL extends, sl_vm_section_t* section)
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