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
sl_define_class2(sl_vm_t* vm, SLID name, SLVAL super);

SLVAL
sl_define_class3(sl_vm_t* vm, SLID name, SLVAL super, SLVAL in);

void
sl_class_set_allocator(sl_vm_t* vm, SLVAL klass, sl_object_t*(*allocator)(sl_vm_t*));

void
sl_define_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)());

void
sl_define_method2(sl_vm_t* vm, SLVAL klass, SLID name, int arity, SLVAL(*func)());

void
sl_define_method3(sl_vm_t* vm, SLVAL klass, SLID name, SLVAL method);

int
sl_class_has_const(sl_vm_t* vm, SLVAL klass, char* name);

int
sl_class_has_const2(sl_vm_t* vm, SLVAL klass, SLID name);

SLVAL
sl_class_get_const(sl_vm_t* vm, SLVAL klass, char* name);

SLVAL
sl_class_get_const2(sl_vm_t* vm, SLVAL klass, SLID name);

void
sl_class_set_const(sl_vm_t* vm, SLVAL klass, char* name, SLVAL val);

void
sl_class_set_const2(sl_vm_t* vm, SLVAL klass, SLID name, SLVAL val);

SLVAL
sl_class_own_instance_methods(sl_vm_t* vm, SLVAL klass);

SLVAL
sl_class_instance_methods(sl_vm_t* vm, SLVAL klass);

int
sl_is_a(struct sl_vm* vm, SLVAL obj, SLVAL klass);

SLVAL
sl_class_of(sl_vm_t* vm, SLVAL obj);

SLVAL
sl_new(sl_vm_t* vm, SLVAL klass, size_t argc, SLVAL* argv);

#endif
