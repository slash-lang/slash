#include "class.h"
#include "value.h"
#include "lib/bignum.h"
#include "lib/float.h"

static int
highest_set_bit(intptr_t t)
{
    int i = 0;
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
    int b, c;
    if(SL_IS_INT(a)) {
        b = sl_get_int(other);
        if(highest_set_bit(a) <= 31 && highest_set_bit(b) <= 31) {
            return sl_make_int(a + b);
        }
        return sl_bignum_add(vm, sl_make_bignum(vm, a), b);
    }
    switch(sl_get_primitive_type(other)) {
        case SL_T_FLOAT:
            return sl_make_float((double)a + sl_get_float(other));
        case SL_T_BIGNUM:
            return sl_bignum_add(vm, other, self);
        default:
            
    }
    (void)vm;
    (void)self;
    (void)other;
}

void
sl_init_int(sl_vm_t* vm)
{
    vm->lib.Int = sl_define_class(vm, "Int", vm->lib.Number);
    sl_define_method(vm, vm->lib.Int, "+", 1, int_add);
}