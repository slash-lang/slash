#ifndef SL_LIB_BIGNUM_H
#define SL_LIB_BIGNUM_H

#include "value.h"
#include "vm.h"

SLVAL
sl_make_bignum(sl_vm_t* vm, int n);

SLVAL
sl_make_bignum_s(sl_vm_t* vm, char* s);

SLVAL
sl_make_bignum_f(sl_vm_t* vm, double n);

long
sl_bignum_get_long(sl_vm_t* vm, SLVAL self);

SLVAL
sl_bignum_to_f(sl_vm_t* vm, SLVAL self);

SLVAL
sl_bignum_to_s(sl_vm_t* vm, SLVAL self);

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

SLVAL
sl_bignum_eq(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_cmp(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_bignum_hash(sl_vm_t* vm, SLVAL self);

#endif
