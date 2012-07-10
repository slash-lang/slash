#ifndef SL_METHOD_H
#define SL_METHOD_H

#include "value.h"
#include "vm.h"

void
sl_init_method(sl_vm_t* vm);

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL name, int arity, SLVAL(*c_func)());

#endif