#ifndef SL_ERROR_H
#define SL_ERROR_H

#include <setjmp.h>
#include "vm.h"

typedef enum sl_unwind_type {
    SL_UNWIND_EXCEPTION = 1,
    SL_UNWIND_EXIT      = 4,
    SL_UNWIND_ALL       = 0xff
}
sl_unwind_type_t;

typedef struct sl_catch_frame {
    jmp_buf env;
    SLVAL value;
    sl_unwind_type_t type;
    struct sl_catch_frame* prev;
}
sl_catch_frame_t;

void
sl_init_error(struct sl_vm* vm);

SLVAL
sl_make_error(struct sl_vm* vm, SLVAL message);

SLVAL
sl_make_error2(struct sl_vm* vm, SLVAL klass, SLVAL message);

void
sl_error_add_frame(struct sl_vm* vm, SLVAL error, SLVAL method, SLVAL file, SLVAL line);

void
sl_unwind(struct sl_vm* vm, SLVAL value, sl_unwind_type_t type);

void
sl_throw(struct sl_vm* vm, SLVAL error);

void
sl_rethrow(struct sl_vm* vm, sl_catch_frame_t* frame);

void
sl_exit(struct sl_vm* vm, SLVAL value);

void
sl_throw_message(struct sl_vm* vm, char* cstr);

void
sl_throw_message2(struct sl_vm* vm, SLVAL klass, char* cstr);

#define SL_TRY(frame, unwind_type, try_block, e, catch_block) do { \
        frame.prev = vm->catch_stack; \
        frame.value = vm->lib.nil; \
        vm->catch_stack = &frame; \
        if(!setjmp(frame.env)) { \
            try_block; \
            vm->catch_stack = frame.prev; \
        } else { \
            vm->catch_stack = frame.prev; \
            if(!(frame.type & (unwind_type))) { \
                sl_rethrow(vm, &frame); \
            } \
            e = frame.value; \
            catch_block; \
        } \
    } while(0)

#define SL_ENSURE(frame, code_block, finally_block) do { \
        frame.prev = vm->catch_stack; \
        frame.value = vm->lib.nil; \
        vm->catch_stack = &frame; \
        if(!setjmp(frame.env)) { \
            code_block; \
            vm->catch_stack = frame.prev; \
            finally_block; \
        } else { \
            vm->catch_stack = frame.prev; \
            finally_block; \
            sl_rethrow(vm, &frame); \
        } \
    } while(0)

#endif
