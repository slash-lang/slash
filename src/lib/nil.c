#include "vm.h"
#include "value.h"
#include "class.h"
#include "string.h"

void
sl_init_nil(sl_vm_t* vm)
{
    sl_object_t* nil = sl_get_ptr(vm->lib.nil);
    vm->lib.Nil = sl_define_class(vm, "Nil", vm->lib.Object);
    nil->klass = vm->lib.Nil;
    nil->primitive_type = SL_T_NIL;
    nil->instance_variables = st_init_table(&sl_string_hash_type);
    nil->singleton_methods = st_init_table(&sl_string_hash_type);
}
