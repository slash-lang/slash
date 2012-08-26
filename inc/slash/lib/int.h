#ifndef SL_LIB_INT_H
#define SL_LIB_INT_H

#include "slash/vm.h"
#include "slash/value.h"

SLVAL
sl_int_to_s(sl_vm_t* vm, SLVAL self);

SLVAL
sl_int_add(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_int_sub(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_int_mul(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_int_div(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_int_mod(sl_vm_t* vm, SLVAL self, SLVAL other);

#endif
