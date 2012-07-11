#include <stdio.h>
#include <stdlib.h>
#include <gc.h>
#include "string.h"
#include "error.h"
#include "value.h"
#include "class.h"
#include "vm.h"

typedef struct sl_error {
    sl_object_t base;
    SLVAL message;
}
sl_error_t;

static sl_object_t*
allocate_error()
{
    return (sl_object_t*)GC_MALLOC(sizeof(sl_error_t));
}

static sl_error_t*
get_error(sl_vm_t* vm, SLVAL object)
{
    sl_expect(vm, object, vm->lib.Error);
    return (sl_error_t*)sl_get_ptr(object);
}

void
sl_init_error(sl_vm_t* vm)
{
    vm->lib.Error = sl_define_class(vm, "Error", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Error, allocate_error);
    vm->lib.SyntaxError = sl_define_class(vm, "SyntaxError", vm->lib.Error);
    vm->lib.EncodingError = sl_define_class(vm, "EncodingError", vm->lib.Error);
    vm->lib.TypeError = sl_define_class(vm, "TypeError", vm->lib.Error);
}

SLVAL
sl_make_error(sl_vm_t* vm, SLVAL message)
{
    return sl_make_error2(vm, vm->lib.Error, message);
}

SLVAL
sl_make_error2(sl_vm_t* vm, SLVAL klass, SLVAL message)
{
    SLVAL err = sl_allocate(vm, klass);
    sl_expect(vm, message, vm->lib.String);
    get_error(vm, err)->message = message;
    return err;
}

void
sl_try(sl_vm_t* vm, void(*try)(sl_vm_t*, void*), void(*catch)(sl_vm_t*, void*, SLVAL), void* state)
{
    sl_catch_frame_t frame;
    frame.prev = vm->catch_stack;
    vm->catch_stack = &frame;
    if(!setjmp(frame.env)) {
        try(vm, state);
    } else {
        catch(vm, state, frame.error);
    }
    vm->catch_stack = frame.prev;
}

void
sl_throw(sl_vm_t* vm, SLVAL error)
{
    if(vm->catch_stack == NULL) {
        fprintf(stderr, "Unhandled exception\n");
        abort();
    }
    vm->catch_stack->error = error;
    longjmp(vm->catch_stack->env, 1);
}

void
sl_throw_message(struct sl_vm* vm, char* cstr)
{
    sl_throw(vm, sl_make_error(vm, sl_make_cstring(vm, cstr)));
}

void
sl_throw_message2(struct sl_vm* vm, SLVAL klass, char* cstr)
{
    sl_throw(vm, sl_make_error2(vm, klass, sl_make_cstring(vm, cstr)));
}