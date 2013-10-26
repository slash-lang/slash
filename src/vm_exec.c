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
#include <string.h>
#include <stdlib.h>

static SLVAL
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL doc, SLVAL extends, sl_vm_section_t* section);

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL doc, sl_vm_section_t* section);

static SLVAL
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLID name, SLVAL doc, sl_vm_section_t* section);

static SLVAL
vm_helper_build_string(sl_vm_t* vm, SLVAL* vals, size_t count);

#define NEXT() (ctx->section->insns[ip++])

#define NEXT_OPCODE()           (NEXT().opcode)
#define NEXT_THREADED_OPCODE()  (NEXT().threaded_opcode)
#define NEXT_IMM()              (NEXT().imm)
#define NEXT_UINT()             (NEXT().uint)
#define NEXT_IMC()              (NEXT().imc)
#define NEXT_ICC()              (NEXT().icc)
#define NEXT_ID()               (NEXT().id)
#define NEXT_REG_IDX()          (NEXT().reg)
#define NEXT_SECTION()          (NEXT().section)

#define NEXT_REG() (ctx->registers[NEXT_REG_IDX()])

#define V_TRUE (vm->lib._true)
#define V_FALSE (vm->lib._false)
#define V_NIL (vm->lib.nil)

typedef struct sl_vm_exception_handler {
    struct sl_vm_exception_handler* prev;
    size_t catch_ip;
}
sl_vm_exception_handler_t;

#ifdef SL_HAS_COMPUTED_GOTO
static sl_vm_exec_ctx_t dummy_ctx;

void*
sl_vm_op_addresses[SL_OP__MAX_OPCODE];
#endif

void
sl_static_init_vm_internals()
{
#ifdef SL_HAS_COMPUTED_GOTO
    sl_vm_exec(&dummy_ctx, 0);
#endif
}

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx, size_t ip)
{
    #ifdef SL_HAS_COMPUTED_GOTO
        // thanks for the tip JavaScriptCore...
        if(sl_unlikely(ctx == &dummy_ctx)) {
            #undef INSTRUCTION
            #define INSTRUCTION(opcode, code) sl_vm_op_addresses[opcode] = &&vm_op_##opcode;
            #include "vm_defn.inc"
            SLVAL dummy = { 0 };
            return dummy;
        }
    #endif

    int line = 0;
    sl_vm_t* vm = ctx->vm;
    sl_vm_exception_handler_t* volatile exception_handler = NULL;
    sl_vm_section_t* section = ctx->section;

    sl_vm_frame_t catch_frame;
    sl_vm_frame_t call_frame;
    call_frame.prev = vm->call_stack;
    call_frame.frame_type = SL_VM_FRAME_SLASH;
    call_frame.as.sl_call_frame.section = section;
    call_frame.as.sl_call_frame.line = (int*)&line;
    vm->call_stack = &call_frame;

    if(ctx->section->has_try_catch) {
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
    }

    #ifdef SL_HAS_COMPUTED_GOTO
        goto *NEXT_THREADED_OPCODE();

        #undef INSTRUCTION
        #define INSTRUCTION(opcode, code) \
            vm_op_##opcode: \
            code; \
            goto *NEXT_THREADED_OPCODE();
        #include "vm_defn.inc"
    #else
        while(1) {
            size_t op;
            switch(op = NEXT_OPCODE()) {
                #undef INSTRUCTION
                #define INSTRUCTION(opcode, code) \
                    case opcode: { \
                        code; \
                    } break;
                #include "vm_defn.inc"

                default:
                    fprintf(stderr, "unknown opcode - %zu!\n", op);
                    abort();
            }
        }
    #endif
}

/* helper functions */

static SLVAL
vm_helper_define_class(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL doc, SLVAL extends, sl_vm_section_t* section)
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
    sl_class_doc_set(ctx->vm, klass, doc);
    sl_vm_exec(subctx, 0);
    return klass;
}

static SLVAL
vm_helper_define_method(sl_vm_exec_ctx_t* ctx, SLID name, SLVAL doc, sl_vm_section_t* section)
{
    SLVAL on = ctx->self, method;
    if(!sl_is_a(ctx->vm, on, ctx->vm->lib.Class)) {
        on = sl_class_of(ctx->vm, on);
    }
    method = sl_make_method(ctx->vm, on, name, section, ctx);
    sl_method_doc_set(ctx->vm, method, doc);
    sl_define_method3(ctx->vm, on, name, method);
    return method;
}

static SLVAL
vm_helper_define_singleton_method(sl_vm_exec_ctx_t* ctx, SLVAL on, SLID name, SLVAL doc, sl_vm_section_t* section)
{
    SLVAL klass = on, method;
    if(!sl_is_a(ctx->vm, klass, ctx->vm->lib.Class)) {
        klass = sl_class_of(ctx->vm, klass);
    }
    method = sl_make_method(ctx->vm, klass, name, section, ctx);
    sl_method_doc_set(ctx->vm, method, doc);
    sl_define_singleton_method3(ctx->vm, on, name, method);
    return method;
}

static SLVAL
vm_helper_build_string(sl_vm_t* vm, SLVAL* vals, size_t count)
{
    for(size_t i = 0; i < count; i++) {
        vals[i] = sl_to_s(vm, vals[i]);
    }
    size_t len = 0;
    for(size_t i = 0; i < count; i++) {
        len += ((sl_string_t*)sl_get_ptr(vals[i]))->buff_len;
    }
    uint8_t* buff = sl_alloc_buffer(vm->arena, len + 1);
    size_t j = 0;
    for(size_t i = 0; i < count; i++) {
        sl_string_t* str = (void*)sl_get_ptr(vals[i]);
        memcpy(buff + j, str->buff, str->buff_len);
        j += str->buff_len;
    }
    return sl_make_string_no_copy(vm, buff, len);
}
