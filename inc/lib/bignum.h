#ifndef SL_LIB_BIGNUM_H
#define SL_LIB_BIGNUM_H

SLVAL
sl_make_bignum(int n);

SLVAL
sl_bignum_add(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_sub(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_mul(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_div(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_mod(sl_vm_t* vm, SLVAL self, SLVAL other);

#endif