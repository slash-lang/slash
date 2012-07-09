#ifndef SL_CLASS_H
#define SL_CLASS_H

#include "vm.h"

void
sl_init_class(sl_vm_t* vm);

SLVAL
sl_define_class(sl_vm_t* vm, char* name, SLVAL super);

SLVAL
sl_define_class2(sl_vm_t* vm, SLVAL name, SLVAL super);

int
sl_is_a(struct sl_vm* vm, SLVAL obj, SLVAL klass);

SLVAL
sl_class_of(sl_vm_t* vm, SLVAL obj);

#endif