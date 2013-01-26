#include <stdlib.h>
#include <slash/st.h>
#include <slash/value.h>
#include <slash/vm.h>
#include <slash/string.h>
#include <slash/class.h>
#include <slash/method.h>
#include <slash/object.h>
#include <slash/mem.h>
#include <slash/lib/array.h>

static sl_object_t*
allocate_class(sl_vm_t* vm)
{
    sl_class_t* klass = sl_alloc(vm->arena, sizeof(sl_class_t));
    klass->base.primitive_type = SL_T_CLASS;
    klass->constants = st_init_table(vm->arena, &sl_id_hash_type);
    klass->class_variables = st_init_table(vm->arena, &sl_id_hash_type);
    klass->instance_methods = st_init_table(vm->arena, &sl_id_hash_type);
    klass->super = vm->lib.Object;
    klass->name.id = 0;
    klass->in = vm->lib.Object;
    return (sl_object_t*)klass;
}

static sl_class_t*
get_class(sl_vm_t* vm, SLVAL klass)
{
    sl_expect(vm, klass, vm->lib.Class);
    return (sl_class_t*)sl_get_ptr(klass);
}

void
sl_pre_init_class(sl_vm_t* vm)
{
    sl_class_t* obj = sl_alloc(vm->arena, sizeof(sl_class_t));
    vm->lib.Class = sl_make_ptr((sl_object_t*)obj);
    obj->name.id = 0;
    obj->super = vm->lib.Object;
    obj->in = vm->lib.Object;
    obj->constants = st_init_table(vm->arena, &sl_id_hash_type);
    obj->class_variables = st_init_table(vm->arena, &sl_id_hash_type);
    obj->instance_methods = st_init_table(vm->arena, &sl_id_hash_type);
    obj->allocator = allocate_class;
    obj->base.klass = vm->lib.Class;
    obj->base.primitive_type = SL_T_CLASS;
    obj->base.instance_variables = st_init_table(vm->arena, &sl_id_hash_type);
}

static SLVAL
sl_class_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_class_t* klass = get_class(vm, self);
    sl_class_t* object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    if(klass == object || sl_get_ptr(klass->in) == (sl_object_t*)object) {
        return sl_id_to_string(vm, get_class(vm, self)->name);
    } else {
        return sl_string_concat(vm,
            sl_class_to_s(vm, klass->in),
            sl_string_concat(vm,
                sl_make_cstring(vm, "::"),
                sl_id_to_string(vm, klass->name)));
    }
}

static SLVAL
sl_class_name(sl_vm_t* vm, SLVAL self)
{
    return sl_id_to_string(vm, get_class(vm, self)->name);
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

static SLVAL
sl_class_instance_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    sl_class_t* klass = get_class(vm, self);
    SLVAL method;
    SLID mid = sl_intern2(vm, method_name);
    method_name = sl_to_s(vm, method_name);
    if(st_lookup(klass->instance_methods, (st_data_t)mid.id, (st_data_t*)&method)) {
        return method;
    } else if(sl_get_primitive_type(klass->super) == SL_T_CLASS) {
        return sl_class_instance_method(vm, klass->super, method_name);
    }
    return vm->lib.nil;
}

static SLVAL
sl_class_own_instance_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    sl_class_t* klass = get_class(vm, self);
    SLVAL method;
    SLID mid = sl_intern2(vm, method_name);
    method_name = sl_to_s(vm, method_name);
    if(st_lookup(klass->instance_methods, (st_data_t)mid.id, (st_data_t*)&method)) {
        return method;
    } else {
        return vm->lib.nil;
    }
}

struct own_instance_methods_iter_state {
    sl_vm_t* vm;
    SLVAL ary;
};

static int
own_instance_methods_iter(SLID name, SLVAL method, struct own_instance_methods_iter_state* state)
{
    (void)method;
    SLVAL namev = sl_id_to_string(state->vm, name);
    sl_array_push(state->vm, state->ary, 1, &namev);
    return ST_CONTINUE;
}

SLVAL
sl_class_own_instance_methods(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* class = get_class(vm, klass);
    SLVAL ary = sl_make_array(vm, 0, NULL);
    struct own_instance_methods_iter_state state;
    state.vm = vm;
    state.ary = ary;
    st_foreach(class->instance_methods, own_instance_methods_iter, (st_data_t)&state);
    return ary;
}

SLVAL
sl_class_instance_methods(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* class = get_class(vm, klass);
    SLVAL ary = sl_class_own_instance_methods(vm, klass);
    while(sl_get_primitive_type(class->super) == SL_T_CLASS) {
        ary = sl_array_concat(vm, ary, sl_class_own_instance_methods(vm, class->super));
        class = (sl_class_t*)sl_get_ptr(class->super);
    }
    return ary;
}

struct class_constants_state {
    sl_vm_t* vm;
    SLVAL ary;
};

static int
class_constants_iter(SLID id, SLVAL value, struct class_constants_state* state)
{
    SLVAL name = sl_id_to_string(state->vm, id);
    sl_array_push(state->vm, state->ary, 1, &name);
    return ST_CONTINUE;
    (void)value;
}

static SLVAL
class_constants(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* class = get_class(vm, klass);
    struct class_constants_state state;
    state.vm = vm;
    state.ary = sl_make_array(vm, 0, NULL);
    st_foreach(class->constants, class_constants_iter, (st_data_t)&state);
    return state.ary;
}

static void
validate_constant_name(sl_vm_t* vm, SLVAL name)
{
    sl_string_t* str = (sl_string_t*)sl_get_ptr(sl_expect(vm, name, vm->lib.String));
    if(str->buff_len == 0 || str->buff[0] < 'A' || str->buff[0] > 'Z') {
        sl_throw_message2(vm, vm->lib.NameError, "Constant names must begin with a capital letter");
    }
}

static SLVAL
class_get_constant(sl_vm_t* vm, SLVAL klass, SLVAL name)
{
    validate_constant_name(vm, name);
    return sl_class_get_const2(vm, klass, sl_intern2(vm, name));
}

static SLVAL
class_set_constant(sl_vm_t* vm, SLVAL klass, SLVAL name, SLVAL value)
{
    validate_constant_name(vm, name);
    sl_class_set_const2(vm, klass, sl_intern2(vm, name), value);
    return klass;
}

static SLVAL
class_remove_constant(sl_vm_t* vm, SLVAL klass, SLVAL name)
{
    validate_constant_name(vm, name);
    sl_class_t* class = get_class(vm, klass);
    SLID id = sl_intern2(vm, name);
    st_delete(class->constants, (st_data_t*)&id.id, NULL);
    return klass;
}

void
sl_init_class(sl_vm_t* vm)
{
    sl_class_t* objectp = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    SLID cid = sl_intern(vm, "Class");
    st_insert(objectp->constants, (st_data_t)cid.id, (st_data_t)sl_get_ptr(vm->lib.Class));

    sl_define_method(vm, vm->lib.Class, "to_s", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "name", 0, sl_class_name);
    sl_define_method(vm, vm->lib.Class, "in", 0, sl_class_in);
    sl_define_method(vm, vm->lib.Class, "super", 0, sl_class_super);
    sl_define_method(vm, vm->lib.Class, "inspect", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "new", -1, sl_new);
    sl_define_method(vm, vm->lib.Class, "instance_method", 1, sl_class_instance_method);
    sl_define_method(vm, vm->lib.Class, "own_instance_method", 1, sl_class_own_instance_method);
    sl_define_method(vm, vm->lib.Class, "own_instance_methods", 0, sl_class_own_instance_methods);
    sl_define_method(vm, vm->lib.Class, "instance_methods", 0, sl_class_instance_methods);
    sl_define_method(vm, vm->lib.Class, "constants", 0, class_constants);
    sl_define_method(vm, vm->lib.Class, "get_constant", 1, class_get_constant);
    sl_define_method(vm, vm->lib.Class, "set_constant", 2, class_set_constant);
    sl_define_method(vm, vm->lib.Class, "remove_constant", 1, class_remove_constant);
}

SLVAL
sl_define_class(sl_vm_t* vm, char* name, SLVAL super)
{
    return sl_define_class2(vm, sl_intern(vm, name), super);
}

SLVAL
sl_define_class2(sl_vm_t* vm, SLID name, SLVAL super)
{
    return sl_define_class3(vm, name, super, vm->lib.Object);
}

SLVAL
sl_define_class3(sl_vm_t* vm, SLID name, SLVAL super, SLVAL in)
{
    SLVAL vklass = sl_make_class(vm, super);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(vklass);
    klass->name = name;
    klass->in = sl_expect(vm, in, vm->lib.Class);
    if(!vm->initializing) {
        st_insert(((sl_class_t*)sl_get_ptr(in))->constants,
            (st_data_t)name.id, (st_data_t)klass);
    }
    return vklass;
}

SLVAL
sl_make_class(sl_vm_t* vm, SLVAL vsuper)
{
    SLVAL vklass = sl_allocate(vm, vm->lib.Class);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(vklass);

    SLVAL vsing = sl_allocate(vm, vm->lib.Class);
    sl_class_t* sing = (sl_class_t*)sl_get_ptr(vsing);

    sl_class_t* super = (sl_class_t*)sl_get_ptr(vsuper);

    sing->allocator = NULL;
    sing->name.id = 0;
    sing->super = super->base.klass;
    sing->in = vm->lib.nil;
    sing->singleton = true;

    klass->base.klass = vsing;
    klass->allocator = super->allocator;
    klass->name.id = 0;
    klass->super = vsuper;
    klass->in = vm->lib.nil;
    klass->singleton = false;

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
    sl_define_method2(vm, klass, sl_intern(vm, name), arity, func);
}

void
sl_define_method2(sl_vm_t* vm, SLVAL klass, SLID name, int arity, SLVAL(*func)())
{
    SLVAL method = sl_make_c_func(vm, klass, name, arity, func);
    sl_define_method3(vm, klass, name, method);
}

void
sl_define_method3(sl_vm_t* vm, SLVAL klass, SLID name, SLVAL method)
{
    sl_expect(vm, method, vm->lib.Method);
    st_insert(get_class(vm, klass)->instance_methods, (st_data_t)name.id, (st_data_t)sl_get_ptr(method));
    vm->state_method++;
}

int
sl_class_has_const(sl_vm_t* vm, SLVAL klass, char* name)
{
    return sl_class_has_const2(vm, klass, sl_intern(vm, name));
}

static int
sl_class_has_own_const(sl_vm_t* vm, SLVAL klass, SLID name)
{
    sl_class_t* klassp;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    return st_lookup(klassp->constants, (st_data_t)name.id, NULL);
}

int
sl_class_has_const2(sl_vm_t* vm, SLVAL klass, SLID name)
{
    sl_class_t* klassp;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    if(st_lookup(klassp->constants, (st_data_t)name.id, NULL)) {
        return 1;
    }
    if(sl_get_primitive_type(klassp->super) == SL_T_CLASS) {
        return sl_class_has_const2(vm, klassp->super, name);
    }
    return 0;
}

SLVAL
sl_class_get_const(sl_vm_t* vm, SLVAL klass, char* name)
{
    return sl_class_get_const2(vm, klass, sl_intern(vm, name));
}

SLVAL
sl_class_get_const2(sl_vm_t* vm, SLVAL klass, SLID name)
{
    sl_class_t* klassp;
    SLVAL val, err;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    if(st_lookup(klassp->constants, (st_data_t)name.id, (st_data_t*)&val)) {
        return val;
    }
    if(sl_get_primitive_type(klassp->super) == SL_T_CLASS && sl_class_has_const2(vm, klassp->super, name)) {
        return sl_class_get_const2(vm, klassp->super, name);
    }
    err = sl_make_cstring(vm, "Undefined constant '");
    err = sl_string_concat(vm, err, sl_id_to_string(vm, name));
    err = sl_string_concat(vm, err, sl_make_cstring(vm, "' in "));
    err = sl_string_concat(vm, err, sl_inspect(vm, klass));
    sl_throw(vm, sl_make_error2(vm, vm->lib.NameError, err));
    return vm->lib.nil; /* never reached */
}

void
sl_class_set_const(sl_vm_t* vm, SLVAL klass, char* name, SLVAL val)
{
    sl_class_set_const2(vm, klass, sl_intern(vm, name), val);
}

void
sl_class_set_const2(sl_vm_t* vm, SLVAL klass, SLID name, SLVAL val)
{
    sl_class_t* klassp;
    SLVAL err;
    if(sl_class_has_own_const(vm, klass, name)) {
        err = sl_make_cstring(vm, "Constant '");
        err = sl_string_concat(vm, err, sl_id_to_string(vm, name));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, "' in "));
        err = sl_string_concat(vm, err, sl_inspect(vm, klass));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, " already defined"));
        sl_throw(vm, sl_make_error2(vm, vm->lib.NameError, err));
    }
    klassp = get_class(vm, klass);
    sl_expect(vm, klass, vm->lib.Class);
    st_insert(klassp->constants, (st_data_t)name.id, (st_data_t)sl_get_ptr(val));
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
        SLVAL klass = sl_get_ptr(obj)->klass;
        sl_class_t* pklass = (sl_class_t*)sl_get_ptr(klass);
        while(pklass->singleton) {
            klass = pklass->super;
            pklass = (sl_class_t*)sl_get_ptr(klass);
        }
        return klass;
    }
}

SLVAL
sl_new(sl_vm_t* vm, SLVAL klass, size_t argc, SLVAL* argv)
{
    SLVAL obj = sl_allocate(vm, klass);
    if(sl_responds_to2(vm, obj, vm->id.init)) {
        sl_send2(vm, obj, vm->id.init, argc, argv);
    }
    return obj;
}
