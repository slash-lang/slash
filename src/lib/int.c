#include <stdio.h>
#include "class.h"
#include "value.h"
#include "lib/bignum.h"
#include "lib/float.h"
#include "string.h"

static int
highest_set_bit(intptr_t t)
{
    int i = 0;
    if(t < 0) t = -t;
    while(t) {
        t >>= 1;
        i++;
    }
    return i;
}

SLVAL
sl_int_add(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    int b;
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_add(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_add(vm, sl_make_float(vm, a), other);
    }
    b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(highest_set_bit(a) < 30 && highest_set_bit(b) < 30) {
        return sl_make_int(vm, a + b);
    } else {
        return sl_bignum_add(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
    }
}

SLVAL
sl_int_sub(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    int b;
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_sub(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_sub(vm, sl_make_float(vm, a), other);
    }
    b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(highest_set_bit(a) < 30 && highest_set_bit(b) < 30) {
        return sl_make_int(vm, a - b);
    } else {
        return sl_bignum_sub(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
    }
}

SLVAL
sl_int_mul(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    int b;
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_mul(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_mul(vm, sl_make_float(vm, a), other);
    }
    b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(highest_set_bit(a) + highest_set_bit(b) < 30) {
        return sl_make_int(vm, a * b);
    } else {
        return sl_bignum_mul(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
    }
}

SLVAL
sl_int_div(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    int b;
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_add(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_add(vm, sl_make_float(vm, a), other);
    }
    b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(b == 0) {
        sl_throw_message2(vm, vm->lib.ZeroDivisionError, "division by zero");
    }
    return sl_make_int(vm, a / b);
}

SLVAL
sl_int_mod(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    int b;
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_mod(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_mod(vm, sl_make_float(vm, a), other);
    }
    b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(b == 0) {
        sl_throw_message2(vm, vm->lib.ZeroDivisionError, "modulo by zero");
    }
    return sl_make_int(vm, a % b);
}

static SLVAL
sl_int_to_s(sl_vm_t* vm, SLVAL self)
{
    int a = sl_get_int(self);
    char buff[32];
    snprintf(buff, 31, "%d", a);
    return sl_make_cstring(vm, buff);
}

static SLVAL
sl_int_to_i(sl_vm_t* vm, SLVAL self)
{
    (void)vm;
    return self;
}

static SLVAL
sl_int_to_f(sl_vm_t* vm, SLVAL self)
{
    return sl_make_float(vm, sl_get_int(self));
}

void
sl_init_int(sl_vm_t* vm)
{
    vm->lib.Int = sl_define_class(vm, "Int", vm->lib.Number);
    sl_define_method(vm, vm->lib.Int, "+", 1, sl_int_add);
    sl_define_method(vm, vm->lib.Int, "-", 1, sl_int_sub);
    sl_define_method(vm, vm->lib.Int, "*", 1, sl_int_mul);
    sl_define_method(vm, vm->lib.Int, "/", 1, sl_int_div);
    sl_define_method(vm, vm->lib.Int, "%", 1, sl_int_mod);
    sl_define_method(vm, vm->lib.Int, "to_s", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "inspect", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "to_i", 0, sl_int_to_i);
    sl_define_method(vm, vm->lib.Int, "to_f", 0, sl_int_to_f);
}
