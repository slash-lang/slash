#ifndef SL_CLASS_H
#define SL_CLASS_H

#include "vm.h"

void
sl_pre_init_class(sl_vm_t* vm);

void
sl_init_class(sl_vm_t* vm);

SLVAL
sl_define_class(sl_vm_t* vm, char* name, SLVAL super);

SLVAL
sl_define_class2(sl_vm_t* vm, SLVAL name, SLVAL super);

void
sl_class_set_allocator(sl_vm_t* vm, SLVAL klass, sl_object_t*(*allocator)(void));

void
sl_define_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)());

void
sl_define_method2(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*func)());

int
sl_class_has_const(sl_vm_t* vm, SLVAL klass, char* name);

int
sl_class_has_const2(sl_vm_t* vm, SLVAL klass, SLVAL name);

SLVAL
sl_class_get_const(sl_vm_t* vm, SLVAL klass, char* name);

SLVAL
sl_class_get_const2(sl_vm_t* vm, SLVAL klass, SLVAL name); 

int
sl_is_a(struct sl_vm* vm, SLVAL obj, SLVAL klass);

SLVAL
sl_class_of(sl_vm_t* vm, SLVAL obj);

#endif
