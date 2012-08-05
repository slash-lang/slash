#include "slash.h"

int
sl_cmp(sl_vm_t* vm, SLVAL a, SLVAL b)
{
    SLVAL ret = sl_send(vm, a, "<=>", 1, b);
    sl_expect(vm, ret, vm->lib.Int);
    return sl_get_int(ret);
}

static SLVAL
comparable_lt(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_cmp(vm, self, other) < 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
comparable_lte(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_cmp(vm, self, other) <= 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
comparable_gt(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_cmp(vm, self, other) > 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
comparable_gte(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_cmp(vm, self, other) >= 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

void
sl_init_comparable(sl_vm_t* vm)
{
    vm->lib.Comparable = sl_define_class(vm, "Comparable", vm->lib.Object);
    sl_define_method(vm, vm->lib.Comparable, "<", 1, comparable_lt);
    sl_define_method(vm, vm->lib.Comparable, "<=", 1, comparable_lte);
    sl_define_method(vm, vm->lib.Comparable, ">", 1, comparable_gt);
    sl_define_method(vm, vm->lib.Comparable, ">=", 1, comparable_gte);
}
