#ifndef SL_OBJECT_H
#define SL_OBJECT_H

#include "vm.h"

void
sl_init_object(sl_vm_t* vm);

void
sl_pre_init_object(sl_vm_t* vm);

SLVAL
sl_to_s(sl_vm_t* vm, SLVAL obj);

char*
sl_to_cstr(sl_vm_t* vm, SLVAL obj);

SLVAL
sl_object_to_s(sl_vm_t* vm, SLVAL self);

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)());

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*func)());

SLVAL
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id);

SLVAL
sl_responds_to2(sl_vm_t* vm, SLVAL object, sl_string_t* id);

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, sl_string_t* id, size_t argc, SLVAL* argv);

#endif
