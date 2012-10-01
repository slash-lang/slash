#include <stdio.h>
#include <slash/class.h>
#include <slash/value.h>
#include <slash/lib/bignum.h>
#include <slash/lib/float.h>
#include <slash/string.h>

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
sl_int_succ(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, sl_get_int(self) + 1);
}

SLVAL
sl_int_pred(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, sl_get_int(self) - 1);
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

SLVAL
sl_int_to_s(sl_vm_t* vm, SLVAL self)
{
    int a = sl_get_int(self);
    char buff[128];
    sprintf(buff, "%d", a);
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

static SLVAL
sl_int_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_eq(vm, sl_make_float(vm, sl_get_int(self)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_eq(vm, sl_make_bignum(vm, sl_get_int(self)), other);
    }
    if(!sl_is_a(vm, other, vm->lib.Int)) {
        return vm->lib._false;
    }
    if(sl_get_int(self) == sl_get_int(other)) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
sl_int_cmp(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_cmp(vm, sl_make_float(vm, sl_get_int(self)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_cmp(vm, sl_make_bignum(vm, sl_get_int(self)), other);
    }
    sl_expect(vm, other, vm->lib.Int);
    if(sl_get_int(self) < sl_get_int(other)) {
        return sl_make_int(vm, -1);
    } else if(sl_get_int(self) > sl_get_int(other)) {
        return sl_make_int(vm, 1);
    } else {
        return sl_make_int(vm, 0);
    }
}

void
sl_init_int(sl_vm_t* vm)
{
    vm->lib.Int = sl_define_class(vm, "Int", vm->lib.Number);
    sl_define_method(vm, vm->lib.Int, "succ", 0, sl_int_succ);
    sl_define_method(vm, vm->lib.Int, "pred", 0, sl_int_pred);
    sl_define_method(vm, vm->lib.Int, "+", 1, sl_int_add);
    sl_define_method(vm, vm->lib.Int, "-", 1, sl_int_sub);
    sl_define_method(vm, vm->lib.Int, "*", 1, sl_int_mul);
    sl_define_method(vm, vm->lib.Int, "/", 1, sl_int_div);
    sl_define_method(vm, vm->lib.Int, "%", 1, sl_int_mod);
    sl_define_method(vm, vm->lib.Int, "to_s", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "inspect", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "to_i", 0, sl_int_to_i);
    sl_define_method(vm, vm->lib.Int, "to_f", 0, sl_int_to_f);
    sl_define_method(vm, vm->lib.Int, "==", 1, sl_int_eq);
    sl_define_method(vm, vm->lib.Int, "<=>", 1, sl_int_cmp);
}
