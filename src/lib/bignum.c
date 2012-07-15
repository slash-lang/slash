#include <stdlib.h>
#include "class.h"
#include "value.h"

static sl_object_t*
allocate_bignum()
{
    return NULL;
}

void
sl_init_bignum(sl_vm_t* vm)
{
    vm->lib.Bignum = sl_define_class(vm, "Bignum", vm->lib.Number);
    sl_class_set_allocator(vm, vm->lib.Bignum, allocate_bignum);
}

SLVAL
sl_make_bignum(sl_vm_t* vm, int n)
{
    (void)vm;
    (void)n;
    /* @TODO */
    abort();
}

SLVAL
sl_bignum_add(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    /* @TODO */
    abort();
    self = other;
    return vm->lib.nil;
}
