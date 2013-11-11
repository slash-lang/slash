#include <stdio.h>
#include <slash/class.h>
#include <slash/value.h>
#include <slash/lib/bignum.h>
#include <slash/lib/float.h>
#include <slash/string.h>
#include <slash/utf8.h>

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
sl_int_negate(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, -sl_get_int(self));
}

SLVAL
sl_int_add(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);

    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        int b = sl_get_int(other);
        if(sl_likely(highest_set_bit(a) < 30 && highest_set_bit(b) < 30)) {
            return sl_make_int(vm, a + b);
        } else {
            return sl_bignum_add(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
        }
    }

    switch(sl_get_primitive_type(other)) {
        case SL_T_BIGNUM:
            return sl_bignum_add(vm, sl_make_bignum(vm, a), other);
        case SL_T_FLOAT:
            return sl_float_add(vm, sl_make_float(vm, a), other);
        default:
            /* force error */
            return sl_expect(vm, other, vm->lib.Int);
    }
}

SLVAL
sl_int_sub(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);

    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        int b = sl_get_int(other);
        if(sl_likely(highest_set_bit(a) < 30 && highest_set_bit(b) < 30)) {
            return sl_make_int(vm, a - b);
        } else {
            return sl_bignum_sub(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
        }
    }

    switch(sl_get_primitive_type(other)) {
        case SL_T_BIGNUM:
            return sl_bignum_sub(vm, sl_make_bignum(vm, a), other);
        case SL_T_FLOAT:
            return sl_float_sub(vm, sl_make_float(vm, a), other);
        default:
            /* force error */
            return sl_expect(vm, other, vm->lib.Int);
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
        return sl_bignum_div(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_div(vm, sl_make_float(vm, a), other);
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
sl_int_pow(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_pow(vm, sl_make_bignum(vm, a), other);
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_pow(vm, sl_make_float(vm, a), other);
    }
    int b = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(highest_set_bit(a) < SL_MAX_INT / b) {
        return sl_bignum_pow(vm, sl_make_bignum(vm, a), sl_make_bignum(vm, b));
    }
    if(b < 0) {
        return sl_float_pow(vm, sl_make_float(vm, a), sl_make_float(vm, b));
    }
    int res = 1;
    while(b) {
        if(b % 2) {
            res *= a;
            b--;
        } else {
            a *= a;
            b /= 2;
        }
    }
    return sl_make_int(vm, res);
}

SLVAL
sl_int_and(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_and(vm, sl_make_bignum(vm, a), other);
    }
    int oth = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    return sl_make_int(vm, a & oth);
}

SLVAL
sl_int_or(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_or(vm, sl_make_bignum(vm, a), other);
    }
    int oth = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    return sl_make_int(vm, a | oth);
}

SLVAL
sl_int_xor(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int a = sl_get_int(self);
    if(sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_xor(vm, sl_make_bignum(vm, a), other);
    }
    int oth = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    return sl_make_int(vm, a ^ oth);
}

SLVAL
sl_int_not(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, ~sl_get_int(self));
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
    return sl_make_bool(vm, sl_get_int(self) == sl_get_int(other));
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

static SLVAL
sl_int_lt(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int res;
    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        res = sl_get_int(self) < sl_get_int(other);
    } else {
        res = sl_cmp(vm, self, other) < 0;
    }
    return res ? vm->lib._true : vm->lib._false;
}

static SLVAL
sl_int_gt(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int res;
    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        res = sl_get_int(self) > sl_get_int(other);
    } else {
        res = sl_cmp(vm, self, other) > 0;
    }
    return res ? vm->lib._true : vm->lib._false;
}

static SLVAL
sl_int_lte(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int res;
    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        res = sl_get_int(self) <= sl_get_int(other);
    } else {
        res = sl_cmp(vm, self, other) <= 0;
    }
    return res ? vm->lib._true : vm->lib._false;
}

static SLVAL
sl_int_gte(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    int res;
    if(sl_likely(sl_get_primitive_type(other) == SL_T_INT)) {
        res = sl_get_int(self) >= sl_get_int(other);
    } else {
        res = sl_cmp(vm, self, other) >= 0;
    }
    return res ? vm->lib._true : vm->lib._false;
}

static SLVAL
sl_int_char(sl_vm_t* vm, SLVAL self)
{
    long n = sl_get_int(self);
    if(n < 0 || n > 0x10FFFF) {
        sl_throw_message2(vm, vm->lib.EncodingError, "not a valid unicode codepoint");
    }
    uint8_t buff[8];
    size_t buff_len = sl_utf32_char_to_utf8(vm, n, buff);
    return sl_make_string(vm, buff, buff_len);
}

void
sl_init_int(sl_vm_t* vm)
{
    vm->lib.Int = sl_define_class(vm, "Int", vm->lib.Number);
    sl_define_method(vm, vm->lib.Int, "succ", 0, sl_int_succ);
    sl_define_method(vm, vm->lib.Int, "pred", 0, sl_int_pred);
    sl_define_method(vm, vm->lib.Int, "-self", 0, sl_int_negate);
    sl_define_method(vm, vm->lib.Int, "+", 1, sl_int_add);
    sl_define_method(vm, vm->lib.Int, "-", 1, sl_int_sub);
    sl_define_method(vm, vm->lib.Int, "*", 1, sl_int_mul);
    sl_define_method(vm, vm->lib.Int, "/", 1, sl_int_div);
    sl_define_method(vm, vm->lib.Int, "%", 1, sl_int_mod);
    sl_define_method(vm, vm->lib.Int, "**", 1, sl_int_pow);
    sl_define_method(vm, vm->lib.Int, "&", 1, sl_int_and);
    sl_define_method(vm, vm->lib.Int, "|", 1, sl_int_or);
    sl_define_method(vm, vm->lib.Int, "^", 1, sl_int_xor);
    sl_define_method(vm, vm->lib.Int, "~self", 0, sl_int_not);
    sl_define_method(vm, vm->lib.Int, "to_s", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "inspect", 0, sl_int_to_s);
    sl_define_method(vm, vm->lib.Int, "to_i", 0, sl_int_to_i);
    sl_define_method(vm, vm->lib.Int, "round", 0, sl_int_to_i);
    sl_define_method(vm, vm->lib.Int, "to_f", 0, sl_int_to_f);
    sl_define_method(vm, vm->lib.Int, "==", 1, sl_int_eq);
    sl_define_method(vm, vm->lib.Int, "<=>", 1, sl_int_cmp);
    sl_define_method(vm, vm->lib.Int, "<", 1, sl_int_lt);
    sl_define_method(vm, vm->lib.Int, ">", 1, sl_int_gt);
    sl_define_method(vm, vm->lib.Int, "<=", 1, sl_int_lte);
    sl_define_method(vm, vm->lib.Int, ">=", 1, sl_int_gte);
    sl_define_method(vm, vm->lib.Int, "char", 0, sl_int_char);
}
