#include "object.h"
#include "class.h"
#include "string.h"

void
sl_init_object(sl_vm_t* vm)
{
    sl_class_t* klass;
    klass = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    klass->super = vm->lib.nil;
    klass->name = vm->lib.nil;
    klass->class_variables = st_init_table(&sl_string_hash_type);
    klass->class_methods = st_init_table(&sl_string_hash_type);
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = st_init_table(&sl_string_hash_type);
}