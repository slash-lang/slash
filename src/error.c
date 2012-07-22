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

static SLVAL
sl_error_name(sl_vm_t* vm, SLVAL self)
{
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(sl_class_of(vm, self));
    return klass->name;
}

static SLVAL
sl_error_message(sl_vm_t* vm, SLVAL self)
{
    sl_error_t* error = get_error(vm, self);
    return error->message;
}

static SLVAL
sl_error_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_error_t* error = get_error(vm, self);
    SLVAL name = sl_error_name(vm, self);
    return sl_string_concat(vm, name,
        sl_string_concat(vm, sl_make_cstring(vm, ": "),
            error->message));
}

void
sl_init_error(sl_vm_t* vm)
{
    vm->lib.Error = sl_define_class(vm, "Error", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Error, allocate_error);
    sl_define_method(vm, vm->lib.Error, "name", 0, sl_error_name);
    sl_define_method(vm, vm->lib.Error, "message", 0, sl_error_message);
    sl_define_method(vm, vm->lib.Error, "to_s", 0, sl_error_to_s);
    
    #define ERROR(klass) vm->lib.klass = sl_define_class(vm, #klass, vm->lib.Error)
    ERROR(ArgumentError);
    ERROR(EncodingError);
    ERROR(NameError);
    ERROR(NoMethodError);
    ERROR(StackOverflowError);
    ERROR(SyntaxError);
    ERROR(TypeError);
    ERROR(ZeroDivisionError);
    #undef ERROR
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
    frame.error = vm->lib.nil;
    vm->catch_stack = &frame;
    if(!setjmp(frame.env)) {
        try(vm, state);
        vm->catch_stack = frame.prev;
    } else {
        vm->catch_stack = frame.prev;
        catch(vm, state, frame.error);
    }
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
