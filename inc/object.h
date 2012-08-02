#ifndef SL_OBJECT_H
#define SL_OBJECT_H

#include "vm.h"

void
sl_init_object(sl_vm_t* vm);

void
sl_pre_init_object(sl_vm_t* vm);

SLVAL
sl_to_s(sl_vm_t* vm, SLVAL obj);

SLVAL
sl_to_s_no_throw(sl_vm_t* vm, SLVAL obj);

SLVAL
sl_inspect(sl_vm_t* vm, SLVAL obj);

char*
sl_to_cstr(sl_vm_t* vm, SLVAL obj);

SLVAL
sl_object_to_s(sl_vm_t* vm, SLVAL self);

SLVAL
sl_object_inspect(sl_vm_t* vm, SLVAL self);

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)());

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*func)());

void
sl_define_singleton_method3(sl_vm_t* vm, SLVAL object, SLVAL name, SLVAL method);

int
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id);

SLVAL
sl_responds_to2(sl_vm_t* vm, SLVAL object, sl_string_t* id);

SLVAL
sl_get_ivar(sl_vm_t* vm, SLVAL object, sl_string_t* id);

SLVAL
sl_get_cvar(sl_vm_t* vm, SLVAL object, sl_string_t* id);

void
sl_set_ivar(sl_vm_t* vm, SLVAL object, sl_string_t* id, SLVAL val);

void
sl_set_cvar(sl_vm_t* vm, SLVAL object, sl_string_t* id, SLVAL val);

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLVAL id, size_t argc, SLVAL* argv);

#endif
