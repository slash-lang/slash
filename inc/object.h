#ifndef SL_OBJECT_H
#define SL_OBJECT_H

#include "vm.h"

void
sl_init_object(sl_vm_t* vm);

void
sl_pre_init_object(sl_vm_t* vm);

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, sl_string_t* id, size_t argc, SLVAL* argv);

#endif