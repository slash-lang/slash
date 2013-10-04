#ifndef SL_ERROR_H
#define SL_ERROR_H

#include <setjmp.h>
#include <slash/platform.h>
#include "vm.h"

typedef enum sl_unwind_type {
    SL_UNWIND_EXCEPTION = 1,
    SL_UNWIND_EXIT      = 4,
    SL_UNWIND_ALL       = 0xff
}
sl_unwind_type_t;

void
sl_init_error(struct sl_vm* vm);

SLVAL
sl_make_error(struct sl_vm* vm, SLVAL message);

SLVAL
sl_make_error2(struct sl_vm* vm, SLVAL klass, SLVAL message);

void
sl_error_add_frame(struct sl_vm* vm, SLVAL error, SLVAL method, SLVAL file, SLVAL line);

void
sl_error_set_message(struct sl_vm* vm, SLVAL error, SLVAL message);

void
sl_unwind(struct sl_vm* vm, SLVAL value, sl_unwind_type_t type);

void
sl_throw(struct sl_vm* vm, SLVAL error);

void
sl_exit(struct sl_vm* vm, SLVAL value);

void
sl_throw_message(struct sl_vm* vm, const char* cstr);

void
sl_throw_message2(struct sl_vm* vm, SLVAL klass, const char* cstr);

void
sl_error(struct sl_vm* vm, SLVAL klass, const char* format, ...);

SLVAL
sl_make_error_fmt(struct sl_vm* vm, SLVAL klass, const char* format, ...);

#define SL_TRY(frame, catch_unwinds, try_block, e, catch_block) do { \
        frame.prev = vm->call_stack; \
        frame.frame_type = SL_VM_FRAME_HANDLER; \
        frame.as.handler_frame.value = vm->lib.nil; \
        vm->call_stack = &frame; \
        if(!sl_setjmp(frame.as.handler_frame.env)) { \
            try_block; \
            vm->call_stack = frame.prev; \
        } else { \
            vm->call_stack = frame.prev; \
            if(!(frame.as.handler_frame.unwind_type & (catch_unwinds))) { \
                sl_unwind(vm, frame.as.handler_frame.value, frame.as.handler_frame.unwind_type); \
            } \
            e = frame.as.handler_frame.value; \
            catch_block; \
        } \
    } while(0)

#define SL_ENSURE(frame, code_block, finally_block) do { \
        frame.prev = vm->call_stack; \
        frame.frame_type = SL_VM_FRAME_HANDLER; \
        vm->call_stack = &frame; \
        if(!sl_setjmp(frame.as.handler_frame.env)) { \
            code_block; \
            vm->call_stack = frame.prev; \
            finally_block; \
        } else { \
            vm->call_stack = frame.prev; \
            finally_block; \
            sl_unwind(vm, frame.as.handler_frame.value, frame.as.handler_frame.unwind_type); \
        } \
    } while(0)

#endif
