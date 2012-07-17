#include "value.h"
#include "vm.h"
#include "class.h"
#include <gc.h>

static sl_object_t*
allocate_false()
{
    sl_object_t* obj = GC_MALLOC(sizeof(sl_object_t));
    obj->primitive_type = SL_T_FALSE;
    return obj;
}

void
sl_init_true(sl_vm_t* vm)
{
    vm->lib.False = sl_define_class(vm, "False", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.False, allocate_false);
    vm->lib._false = sl_allocate(vm, vm->lib.False);
}
