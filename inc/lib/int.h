#ifndef SL_LIB_INT_H
#define SL_LIB_INT_H

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
