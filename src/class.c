#include <gc.h>
#include "st.h"
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"

static sl_object_t*
sl_class_allocator()
{
    sl_class_t* klass = GC_MALLOC(sizeof(sl_class_t));
    klass->base.primitive_type = SL_T_CLASS;
    return (sl_object_t*)klass;
}

void
sl_init_class(sl_vm_t* vm)
{
    sl_class_t* obj = GC_MALLOC(sizeof(sl_class_t));
    vm->lib.Class = sl_make_ptr((sl_object_t*)obj);
    obj->name = vm->lib.nil;
    obj->super = vm->lib.Object;
    obj->class_variables = st_init_table(&sl_string_hash_type);
    obj->class_methods = st_init_table(&sl_string_hash_type);
    obj->instance_methods = st_init_table(&sl_string_hash_type);
    obj->allocator = sl_class_allocator;
    obj->base.klass = vm->lib.Class;
    obj->base.primitive_type = SL_T_CLASS;
    obj->base.instance_variables = st_init_table(&sl_string_hash_type);
}

SLVAL
sl_define_class(sl_vm_t* vm, char* name, SLVAL super)
{
    return sl_define_class2(vm, sl_make_cstring(vm, name), super);
}

SLVAL
sl_define_class2(sl_vm_t* vm, SLVAL name, SLVAL super)
{
    SLVAL vklass = sl_allocate(vm, vm->lib.Class);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(vklass);
    /* @TODO assert super is a class */
    klass->super = super;
    /* @TODO assert name is a string */
    klass->name = name;
    klass->class_variables = st_init_table(&sl_string_hash_type);
    klass->class_methods = st_init_table(&sl_string_hash_type);
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    return vklass;
}

int
sl_is_a(sl_vm_t* vm, SLVAL obj, SLVAL klass)
{
    SLVAL vk = sl_class_of(vm, obj);
    while(1) {
        if(vk.i == klass.i) {
            return 1;
        }
        if(vk.i == vm->lib.Object.i) {
            return 0;
        }
        vk = ((sl_class_t*)sl_get_ptr(vk))->super;
    }
}

SLVAL
sl_class_of(sl_vm_t* vm, SLVAL obj)
{
    if(sl_get_primitive_type(obj) == SL_T_INT) {
        return vm->lib.Int;
    } else {
        return sl_get_ptr(obj)->klass;
    }
}