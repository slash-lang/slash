#include "slash/value.h"
#include "slash/vm.h"
#include "slash/class.h"
#include "slash/string.h"

static sl_object_t*
allocate_true(sl_vm_t* vm)
{
    sl_object_t* obj = sl_alloc(vm->arena, sizeof(sl_object_t));
    obj->primitive_type = SL_T_TRUE;
    return obj;
}

static SLVAL
true_to_s(sl_vm_t* vm)
{
    return sl_make_cstring(vm, "true");
}

static SLVAL
true_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    (void)self;
    if(sl_is_a(vm, other, vm->lib.True)) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

void
sl_init_true(sl_vm_t* vm)
{
    vm->lib.True = sl_define_class(vm, "True", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.True, allocate_true);
    sl_define_method(vm, vm->lib.True, "to_s", 0, true_to_s);
    sl_define_method(vm, vm->lib.True, "inspect", 0, true_to_s);
    sl_define_method(vm, vm->lib.True, "==", 1, true_eq);
    vm->lib._true = sl_allocate(vm, vm->lib.True);
}
