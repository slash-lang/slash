#include <stdlib.h>
#include "class.h"
#include "value.h"
#include "lib/float.h"
#include "lib/bignum.h"
#include "string.h"
#include <gmp.h>
#include <gc.h>

typedef struct {
    sl_object_t base;
    mpz_t mpz;
}
sl_bignum_t;

static void
free_bignum(sl_bignum_t* bn)
{
    mpz_clear(bn->mpz);
}

static sl_object_t*
allocate_bignum()
{
    sl_bignum_t* bn = (sl_bignum_t*)GC_MALLOC(sizeof(sl_bignum_t));
    bn->base.primitive_type = SL_T_BIGNUM;
    GC_register_finalizer(bn, (void(*)(void*,void*))free_bignum, NULL, NULL, NULL);
    mpz_init(bn->mpz);
    return (sl_object_t*)bn;
}

static sl_bignum_t*
get_bignum(sl_vm_t* vm, SLVAL val)
{
    sl_expect(vm, val, vm->lib.Bignum);
    return (sl_bignum_t*)sl_get_ptr(val);
}

static SLVAL
sl_bignum_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_bignum_t* bn = get_bignum(vm, self);
    size_t dig = mpz_sizeinbase(bn->mpz, 10);
    char* str = GC_MALLOC_ATOMIC(dig + 4);
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

void
sl_init_bignum(sl_vm_t* vm)
{
    vm->lib.Bignum = sl_define_class(vm, "Bignum", vm->lib.Comparable);
    sl_class_set_allocator(vm, vm->lib.Bignum, allocate_bignum);
    sl_define_method(vm, vm->lib.Bignum, "to_s", 0, sl_bignum_to_s);
    sl_define_method(vm, vm->lib.Bignum, "inspect", 0, sl_bignum_to_s);
    sl_define_method(vm, vm->lib.Bignum, "to_i", 0, sl_bignum_to_i);
    sl_define_method(vm, vm->lib.Bignum, "to_f", 0, sl_bignum_to_f);
    sl_define_method(vm, vm->lib.Bignum, "succ", 0, sl_bignum_succ);
    sl_define_method(vm, vm->lib.Bignum, "+", 1, sl_bignum_add);
    sl_define_method(vm, vm->lib.Bignum, "-", 1, sl_bignum_sub);
    sl_define_method(vm, vm->lib.Bignum, "*", 1, sl_bignum_mul);
    sl_define_method(vm, vm->lib.Bignum, "/", 1, sl_bignum_div);
    sl_define_method(vm, vm->lib.Bignum, "%", 1, sl_bignum_mod);
    sl_define_method(vm, vm->lib.Bignum, "==", 1, sl_bignum_eq);
    sl_define_method(vm, vm->lib.Bignum, "<=>", 1, sl_bignum_cmp);
}

SLVAL
sl_make_bignum(sl_vm_t* vm, int n)
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
        return sl_bignum_mul(vm, self, sl_make_bignum(vm, sl_get_int(other)));
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
sl_bignum_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Int)) {
        return sl_bignum_eq(vm, self, sl_make_bignum(vm, sl_get_int(other)));
    }
    if(sl_is_a(vm, other, vm->lib.Float)) {
        if(mpz_cmp_d(get_bignum(vm, self)->mpz, sl_get_float(vm, other)) == 0) {
            return vm->lib._true;
        } else {
            return vm->lib._false;
        }
    }
    if(!sl_is_a(vm, other, vm->lib.Bignum)) {
        return vm->lib._false;
    }
    if(mpz_cmp(get_bignum(vm, self)->mpz, get_bignum(vm, other)->mpz) == 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
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
    return sl_make_int(vm, mpz_cmp(get_bignum(vm, self)->mpz, get_bignum(vm, other)->mpz));
}