#include <slash/value.h>
#include <slash/vm.h>
#include <slash/class.h>
#include <slash/string.h>

static sl_object_t*
allocate_false(sl_vm_t* vm)
{
    sl_object_t* obj = sl_alloc(vm->arena, sizeof(sl_object_t));
    obj->primitive_type = SL_T_FALSE;
    return obj;
}

static SLVAL
false_to_s(sl_vm_t* vm)
{
    return sl_make_cstring(vm, "false");
}

static SLVAL
false_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    (void)self;
    if(sl_is_a(vm, other, vm->lib.False)) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

void
sl_init_false(sl_vm_t* vm)
{
    vm->lib.False = sl_define_class(vm, "False", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.False, allocate_false);
    sl_define_method(vm, vm->lib.False, "to_s", 0, false_to_s);
    sl_define_method(vm, vm->lib.False, "inspect", 0, false_to_s);
    sl_define_method(vm, vm->lib.False, "==", 1, false_eq);
    vm->lib._false = sl_allocate(vm, vm->lib.False);
}
