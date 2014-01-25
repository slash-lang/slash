#include <stdlib.h>
#include <slash/class.h>
#include <slash/value.h>
#include <slash/lib/float.h>
#include <slash/lib/bignum.h>
#include <slash/string.h>

static void
free_bignum(void* ptr)
{
    sl_bignum_t* bn = ptr;
    mpz_clear(bn->mpz);
}

static sl_gc_shape_t
bignum_shape = {
    .mark     = sl_gc_conservative_mark,
    .finalize = free_bignum,
};

static sl_object_t*
allocate_bignum(sl_vm_t* vm)
{
    sl_bignum_t* bn = sl_alloc2(vm->arena, &bignum_shape, sizeof(sl_bignum_t));
    bn->base.primitive_type = SL_T_BIGNUM;
    mpz_init(bn->mpz);
    return (sl_object_t*)bn;
}

static sl_bignum_t*
get_bignum(sl_vm_t* vm, SLVAL val)
{
    sl_expect(vm, val, vm->lib.Bignum);
    return (sl_bignum_t*)sl_get_ptr(val);
}

long
sl_bignum_get_long(sl_vm_t* vm, SLVAL self)
{
    sl_bignum_t* bn = get_bignum(vm, self);
    return mpz_get_si(bn->mpz);
}

SLVAL
sl_bignum_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_bignum_t* bn = get_bignum(vm, self);
    size_t dig = mpz_sizeinbase(bn->mpz, 10);
    char* str = sl_alloc_buffer(vm->arena, dig + 4);
    gmp_snprintf(str, dig + 3, "%Zd", bn->mpz);
    return sl_make_cstring(vm, str);
}

static SLVAL
sl_bignum_to_i(sl_vm_t* vm, SLVAL val)
{
    (void)vm;
    return val;
}

static SLVAL
sl_bignum_succ(sl_vm_t* vm, SLVAL val)
{
    sl_bignum_t* a = get_bignum(vm, val);
    SLVAL b = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bp = get_bignum(vm, b);
    mpz_add_ui(bp->mpz, a->mpz, 1);
    return b;
}

static SLVAL
sl_bignum_pred(sl_vm_t* vm, SLVAL val)
{
    sl_bignum_t* a = get_bignum(vm, val);
    SLVAL b = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bp = get_bignum(vm, b);
    mpz_sub_ui(bp->mpz, a->mpz, 1);
    return b;
}

static SLVAL
sl_bignum_negate(sl_vm_t* vm, SLVAL val)
{
    sl_bignum_t* a = get_bignum(vm, val);
    SLVAL b = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bp = get_bignum(vm, b);
    mpz_neg(bp->mpz, a->mpz);
    return b;
}

void
sl_init_bignum(sl_vm_t* vm)
{
    vm->lib.Bignum = sl_define_class(vm, "Bignum", vm->lib.Number);
    sl_class_set_allocator(vm, vm->lib.Bignum, allocate_bignum);
    sl_define_method(vm, vm->lib.Bignum, "to_s", 0, sl_bignum_to_s);
    sl_define_method(vm, vm->lib.Bignum, "inspect", 0, sl_bignum_to_s);
    sl_define_method(vm, vm->lib.Bignum, "to_i", 0, sl_bignum_to_i);
    sl_define_method(vm, vm->lib.Bignum, "round", 0, sl_bignum_to_i);
    sl_define_method(vm, vm->lib.Bignum, "to_f", 0, sl_bignum_to_f);
    sl_define_method(vm, vm->lib.Bignum, "succ", 0, sl_bignum_succ);
    sl_define_method(vm, vm->lib.Bignum, "pred", 0, sl_bignum_pred);
    sl_define_method(vm, vm->lib.Bignum, "-self", 0, sl_bignum_negate);
    sl_define_method(vm, vm->lib.Bignum, "+", 1, sl_bignum_add);
    sl_define_method(vm, vm->lib.Bignum, "-", 1, sl_bignum_sub);
    sl_define_method(vm, vm->lib.Bignum, "*", 1, sl_bignum_mul);
    sl_define_method(vm, vm->lib.Bignum, "/", 1, sl_bignum_div);
    sl_define_method(vm, vm->lib.Bignum, "%", 1, sl_bignum_mod);
    sl_define_method(vm, vm->lib.Bignum, "**", 1, sl_bignum_pow);
    sl_define_method(vm, vm->lib.Bignum, "&", 1, sl_bignum_and);
    sl_define_method(vm, vm->lib.Bignum, "|", 1, sl_bignum_or);
    sl_define_method(vm, vm->lib.Bignum, "^", 1, sl_bignum_xor);
    sl_define_method(vm, vm->lib.Bignum, "==", 1, sl_bignum_eq);
    sl_define_method(vm, vm->lib.Bignum, "<=>", 1, sl_bignum_cmp);
    sl_define_method(vm, vm->lib.Bignum, "hash", 0, sl_bignum_hash);
}

SLVAL
sl_make_bignum(sl_vm_t* vm, long n)
{
    SLVAL bnv = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bn = get_bignum(vm, bnv);
    mpz_set_si(bn->mpz, n);
    return bnv;
}

SLVAL
sl_make_bignum_s(sl_vm_t* vm, char* s)
{
    SLVAL bnv = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bn = get_bignum(vm, bnv);
    mpz_set_str(bn->mpz, s, 10);
    return bnv;
}

SLVAL
sl_make_bignum_f(sl_vm_t* vm, double n)
{
    SLVAL bnv = sl_allocate(vm, vm->lib.Bignum);
    sl_bignum_t* bn = get_bignum(vm, bnv);
    mpz_set_d(bn->mpz, n);
    return bnv;
}

SLVAL
sl_bignum_to_f(sl_vm_t* vm, SLVAL self)
{
    double d = mpz_get_d(get_bignum(vm, self)->mpz);
    return sl_make_float(vm, d);
}

SLVAL
sl_bignum_add(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_add(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_add(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_add(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_sub(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_sub(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_sub(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_sub(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_mul(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_mul(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_mul(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_mul(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_div(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_div(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_div(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    b = get_bignum(vm, other);
    if(mpz_sgn(b->mpz) == 0) {
        sl_throw_message2(vm, vm->lib.ZeroDivisionError, "division by zero");
    }
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_tdiv_q(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_mod(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_mod(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_mod(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    b = get_bignum(vm, other);
    if(mpz_sgn(b->mpz) == 0) {
        sl_throw_message2(vm, vm->lib.ZeroDivisionError, "modulo by zero");
    }
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_tdiv_r(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_pow(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_float_pow(vm, sl_make_float(vm, mpz_get_d(a->mpz)), other);
    }
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_pow(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    sl_bignum_t* b = get_bignum(vm, other);
    if(mpz_sgn(b->mpz) < 0) {
        return sl_float_pow(vm, sl_bignum_to_f(vm, self), sl_bignum_to_f(vm, other));
    }
    if(mpz_cmp_si(b->mpz, LONG_MAX) > 0) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "exponent too large");
    }
    sl_bignum_t* c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_pow_ui(c->mpz, a->mpz, mpz_get_si(b->mpz));
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_and(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(!sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_and(vm, self, sl_make_bignum(vm, sl_get_int(sl_expect(vm, other, vm->lib.Int))));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_and(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_or(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(!sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_or(vm, self, sl_make_bignum(vm, sl_get_int(sl_expect(vm, other, vm->lib.Int))));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_ior(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_xor(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_bignum_t* a = get_bignum(vm, self);
    sl_bignum_t* b;
    sl_bignum_t* c;
    if(!sl_is_a(vm, other, vm->lib.Bignum)) {
        return sl_bignum_xor(vm, self, sl_make_bignum(vm, sl_get_int(sl_expect(vm, other, vm->lib.Int))));
    }
    b = get_bignum(vm, other);
    c = get_bignum(vm, sl_allocate(vm, vm->lib.Bignum));
    mpz_xor(c->mpz, a->mpz, b->mpz);
    return sl_make_ptr((sl_object_t*)c);
}

SLVAL
sl_bignum_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_eq(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_make_bool(vm, mpz_cmp_d(get_bignum(vm, self)->mpz, sl_get_float(vm, other)) == 0);
    }
    if(!sl_is_a(vm, other, vm->lib.Bignum)) {
        return vm->lib._false;
    }
    return sl_make_bool(vm, mpz_cmp(get_bignum(vm, self)->mpz, get_bignum(vm, other)->mpz) == 0);
}

SLVAL
sl_bignum_cmp(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_cmp(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        return sl_make_int(vm, mpz_cmp_d(get_bignum(vm, self)->mpz, sl_get_float(vm, other)));
    }
    sl_expect(vm, other, vm->lib.Bignum);
    int cmp = mpz_cmp(get_bignum(vm, self)->mpz, get_bignum(vm, other)->mpz);
    /* we'd like to normalize this: */
    if(cmp > 0) {
        cmp = 1;
    } else if(cmp < 0) {
        cmp = -1;
    }
    return sl_make_int(vm, cmp);
}

SLVAL
sl_bignum_hash(sl_vm_t* vm, SLVAL self)
{
    sl_bignum_t* bn = get_bignum(vm, self);
    unsigned long i = mpz_get_ui(bn->mpz);
    if(i >= SL_MAX_INT) {
        i %= SL_MAX_INT;
    }
    return sl_make_int(vm, i);
}
