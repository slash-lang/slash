#include <stdlib.h>
#include <gc.h>
#include "st.h"
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"
#include "method.h"

static sl_object_t*
allocate_class()
{
    sl_class_t* klass = GC_MALLOC(sizeof(sl_class_t));
    klass->base.primitive_type = SL_T_CLASS;
    return (sl_object_t*)klass;
}

static sl_class_t*
get_class(sl_vm_t* vm, SLVAL klass)
{
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        /* @TODO error */
        abort();
    }
    return (sl_class_t*)sl_get_ptr(klass);
}

void
sl_pre_init_class(sl_vm_t* vm)
{
    sl_class_t* obj = GC_MALLOC(sizeof(sl_class_t));
    vm->lib.Class = sl_make_ptr((sl_object_t*)obj);
    obj->name = vm->lib.nil;
    obj->super = vm->lib.Object;
    obj->class_variables = st_init_table(&sl_string_hash_type);
    obj->instance_methods = st_init_table(&sl_string_hash_type);
    obj->allocator = allocate_class;
    obj->base.klass = vm->lib.Class;
    obj->base.primitive_type = SL_T_CLASS;
    obj->base.instance_variables = st_init_table(&sl_string_hash_type);
}

static SLVAL
sl_class_to_s(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->name;
}

void
sl_init_class(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Class, "to_s", 0, sl_class_to_s);
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
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    klass->allocator = get_class(vm, super)->allocator;
    return vklass;
}

void
sl_class_set_allocator(sl_vm_t* vm, SLVAL klass, sl_object_t*(*allocator)(void))
{
    get_class(vm, klass)->allocator = allocator;
}

void
sl_define_method(sl_vm_t* vm, SLVAL klass, char* name, int arity, SLVAL(*func)())
{
    sl_define_method2(vm, klass, sl_make_cstring(vm, name), arity, func);
}

void
sl_define_method2(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*func)())
{
    SLVAL method = sl_make_c_func(vm, name, arity, func);
    st_insert(get_class(vm, klass)->instance_methods, (st_data_t)sl_get_ptr(name), (st_data_t)sl_get_ptr(method));
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
