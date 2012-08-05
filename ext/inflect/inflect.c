#include "slash.h"

static SLVAL
ordinalize(sl_vm_t* vm, int rem)
{
    char* suffix;
    switch(rem) {
        case 1:
            suffix = "st";
            break;
        case 2:
            suffix = "nd";
            break;
        case 3:
            suffix = "rd";
            break;
        default:
            suffix = "th";
            break;
    }
    return sl_make_cstring(vm, suffix);
}

static SLVAL
int_ordinalize(sl_vm_t* vm, SLVAL self)
{
    int i = sl_get_int(self);
    return sl_string_concat(vm, sl_to_s(vm, self), ordinalize(vm, i % 10));
}

static SLVAL
bignum_ordinalize(sl_vm_t* vm, SLVAL self)
{
    long rem = sl_bignum_get_long(vm, sl_bignum_mod(vm, self, sl_make_int(vm, 10)));
    return sl_string_concat(vm, sl_to_s(vm, self), ordinalize(vm, rem % 10));
}

void
sl_ext_inflect_pluralize_init(sl_vm_t* vm);

void
sl_ext_inflect_pluralize_static_init();

void
sl_init_ext_inflect(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Int, "ordinalize", 0, int_ordinalize);
    sl_define_method(vm, vm->lib.Bignum, "ordinalize", 0, bignum_ordinalize);
    sl_ext_inflect_pluralize_init(vm);
}

void
sl_static_init_ext_inflect()
{
    sl_ext_inflect_pluralize_static_init();
}
