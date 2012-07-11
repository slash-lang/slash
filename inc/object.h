#ifndef SL_OBJECT_H
#define SL_OBJECT_H

#include "vm.h"

void
sl_init_object(sl_vm_t* vm);

void
sl_pre_init_object(sl_vm_t* vm);

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)());

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*func)());

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, sl_string_t* id, size_t argc, SLVAL* argv);

#endif