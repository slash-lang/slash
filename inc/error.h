#ifndef SL_ERROR_H
#define SL_ERROR_H

#include <setjmp.h>
#include "vm.h"

typedef struct sl_catch_frame {
    jmp_buf env;
    SLVAL error;
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
sl_try(struct sl_vm* vm, void(*try)(struct sl_vm*, void*), void(*catch)(struct sl_vm*, void*, SLVAL), void* state);

void
sl_throw(struct sl_vm* vm, SLVAL error);

void
sl_throw_message(struct sl_vm* vm, char* cstr);

void
sl_throw_message2(struct sl_vm* vm, SLVAL klass, char* cstr);

#endif
