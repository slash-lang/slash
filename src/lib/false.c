#include "value.h"
#include "vm.h"
#include "class.h"
#include "string.h"
#include <gc.h>

static sl_object_t*
allocate_false()
{
    sl_object_t* obj = GC_MALLOC(sizeof(sl_object_t));
    obj->primitive_type = SL_T_FALSE;
    return obj;
}

static SLVAL
false_to_s(sl_vm_t* vm)
{
    return sl_make_cstring(vm, "false");
}

void
sl_init_false(sl_vm_t* vm)
{
    vm->lib.False = sl_define_class(vm, "False", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.False, allocate_false);
    sl_define_method(vm, vm->lib.False, "to_s", 0, false_to_s);
    sl_define_method(vm, vm->lib.False, "inspect", 0, false_to_s);
    vm->lib._false = sl_allocate(vm, vm->lib.False);
}
