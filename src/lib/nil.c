#include "vm.h"
#include "value.h"
#include "class.h"
#include "string.h"

static SLVAL
nil_to_s(sl_vm_t* vm)
{
    return sl_make_cstring(vm, "");
}

static SLVAL
nil_inspect(sl_vm_t* vm)
{
    return sl_make_cstring(vm, "nil");
}

void
sl_init_nil(sl_vm_t* vm)
{
    sl_object_t* nil = sl_get_ptr(vm->lib.nil);
    vm->lib.Nil = sl_define_class(vm, "Nil", vm->lib.Object);
    nil->klass = vm->lib.Nil;
    nil->primitive_type = SL_T_NIL;
    nil->instance_variables = st_init_table(&sl_string_hash_type);
    nil->singleton_methods = st_init_table(&sl_string_hash_type);
    
    sl_define_method(vm, vm->lib.Nil, "to_s", 0, nil_to_s);
    sl_define_method(vm, vm->lib.Nil, "inspect", 0, nil_inspect);
}
