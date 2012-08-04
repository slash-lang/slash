#include <stdlib.h>
#include <gc.h>
#include "st.h"
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"
#include "method.h"
#include "object.h"
#include "mem.h"

static sl_object_t*
allocate_class(sl_vm_t* vm)
{
    sl_class_t* klass = sl_alloc(vm->arena, sizeof(sl_class_t));
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
    sl_class_t* obj = sl_alloc(vm->arena, sizeof(sl_class_t));
    vm->lib.Class = sl_make_ptr((sl_object_t*)obj);
    obj->name = vm->lib.nil;
    obj->super = vm->lib.Object;
    obj->in = vm->lib.Object;
    obj->constants = st_init_table(vm->arena, &sl_string_hash_type);
    obj->class_variables = st_init_table(vm->arena, &sl_string_hash_type);
    obj->instance_methods = st_init_table(vm->arena, &sl_string_hash_type);
    obj->allocator = allocate_class;
    obj->base.klass = vm->lib.Class;
    obj->base.primitive_type = SL_T_CLASS;
    obj->base.instance_variables = st_init_table(vm->arena, &sl_string_hash_type);
}

static SLVAL
sl_class_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_class_t* klass = get_class(vm, self);
    sl_class_t* object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    if(klass == object || sl_get_ptr(klass->in) == (sl_object_t*)object) {
        return get_class(vm, self)->name;
    } else {
        return sl_string_concat(vm,
            sl_class_to_s(vm, klass->in),
            sl_string_concat(vm,
                sl_make_cstring(vm, "::"),
                klass->name));
    }
}

static SLVAL
sl_class_name(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->name;
}

static SLVAL
sl_class_in(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->in;
}

static SLVAL
sl_class_super(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->super;
}

void
sl_init_class(sl_vm_t* vm)
{
    st_insert(((sl_class_t*)sl_get_ptr(vm->lib.Object))->constants,
        (st_data_t)sl_cstring(vm, "Class"), (st_data_t)vm->lib.Class.i);
    sl_define_method(vm, vm->lib.Class, "to_s", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "name", 0, sl_class_name);
    sl_define_method(vm, vm->lib.Class, "in", 0, sl_class_in);
    sl_define_method(vm, vm->lib.Class, "super", 0, sl_class_super);
    sl_define_method(vm, vm->lib.Class, "inspect", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "new", -1, sl_new);
}

SLVAL
sl_define_class(sl_vm_t* vm, char* name, SLVAL super)
{
    return sl_define_class2(vm, sl_make_cstring(vm, name), super);
}

SLVAL
sl_define_class2(sl_vm_t* vm, SLVAL name, SLVAL super)
{
    return sl_define_class3(vm, name, super, vm->lib.Object);
}

SLVAL
sl_define_class3(sl_vm_t* vm, SLVAL name, SLVAL super, SLVAL in)
{
    SLVAL vklass = sl_allocate(vm, vm->lib.Class);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(vklass);
    klass->super = sl_expect(vm, super, vm->lib.Class);
    klass->name = sl_expect(vm, name, vm->lib.String);
    klass->in = sl_expect(vm, in, vm->lib.Class);
    klass->constants = st_init_table(vm->arena, &sl_string_hash_type);
    klass->class_variables = st_init_table(vm->arena, &sl_string_hash_type);
    klass->instance_methods = st_init_table(vm->arena, &sl_string_hash_type);
    klass->allocator = get_class(vm, super)->allocator;
    if(!vm->initializing) {
        st_insert(((sl_class_t*)sl_get_ptr(in))->constants,
            (st_data_t)name.i, (st_data_t)vklass.i);
    }
    return vklass;
}

void
sl_class_set_allocator(sl_vm_t* vm, SLVAL klass, sl_object_t*(*allocator)(sl_vm_t*))
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
    SLVAL method = sl_make_c_func(vm, klass, name, arity, func);
    sl_define_method3(vm, klass, name, method);
}

void
sl_define_method3(sl_vm_t* vm, SLVAL klass, SLVAL name, SLVAL method)
{
    sl_expect(vm, name, vm->lib.String);
    sl_expect(vm, method, vm->lib.Method);
    st_insert(get_class(vm, klass)->instance_methods, (st_data_t)sl_get_ptr(name), (st_data_t)sl_get_ptr(method));
}

int
sl_class_has_const(sl_vm_t* vm, SLVAL klass, char* name)
{
    return sl_class_has_const2(vm, klass, sl_make_cstring(vm, name));
}

int
sl_class_has_const2(sl_vm_t* vm, SLVAL klass, SLVAL name)
{
    sl_class_t* klassp;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    return st_lookup(klassp->constants, (st_data_t)sl_get_ptr(name), NULL);
}

SLVAL
sl_class_get_const(sl_vm_t* vm, SLVAL klass, char* name)
{
    return sl_class_get_const2(vm, klass, sl_make_cstring(vm, name));
}

SLVAL
sl_class_get_const2(sl_vm_t* vm, SLVAL klass, SLVAL name)
{
    sl_class_t* klassp;
    SLVAL val, err;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    if(st_lookup(klassp->constants, (st_data_t)sl_get_ptr(name), (st_data_t*)&val)) {
        return val;
    }
    err = sl_make_cstring(vm, "Undefined constant '");
    err = sl_string_concat(vm, err, name);
    err = sl_string_concat(vm, err, sl_make_cstring(vm, "' in "));
    err = sl_string_concat(vm, err, sl_inspect(vm, klass));
    sl_throw(vm, sl_make_error2(vm, vm->lib.NameError, err));
    return vm->lib.nil;
}

void
sl_class_set_const(sl_vm_t* vm, SLVAL klass, char* name, SLVAL val)
{
    sl_class_set_const2(vm, klass, sl_make_cstring(vm, name), val);
}

void
sl_class_set_const2(sl_vm_t* vm, SLVAL klass, SLVAL name, SLVAL val)
{
    sl_class_t* klassp;
    SLVAL err;
    if(sl_class_has_const2(vm, klass, name)) {
        err = sl_make_cstring(vm, "Constant '");
        err = sl_string_concat(vm, err, name);
        err = sl_string_concat(vm, err, sl_make_cstring(vm, "' in "));
        err = sl_string_concat(vm, err, sl_inspect(vm, klass));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, " already defined"));
        sl_throw(vm, sl_make_error2(vm, vm->lib.NameError, err));
    }
    klassp = get_class(vm, klass);
    sl_expect(vm, klass, vm->lib.Class);
    st_insert(klassp->constants, (st_data_t)sl_get_ptr(name), (st_data_t)sl_get_ptr(val));
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

SLVAL
sl_new(sl_vm_t* vm, SLVAL klass, size_t argc, SLVAL* argv)
{
    SLVAL obj = sl_allocate(vm, klass);
    if(sl_responds_to(vm, obj, "init")) {
        sl_send2(vm, obj, sl_make_cstring(vm, "init"), argc, argv);
    }
    return obj;
}
