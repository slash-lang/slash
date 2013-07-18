#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <slash/object.h>
#include <slash/class.h>
#include <slash/string.h>
#include <slash/method.h>
#include <slash/platform.h>
#include <slash/lib/array.h>

void
sl_pre_init_object(sl_vm_t* vm)
{
    sl_class_t* klass;
    klass = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    klass->super = vm->lib.nil;
    klass->name.id = 0;
    klass->in = vm->lib.nil;
    klass->doc = vm->lib.nil;
    klass->constants = sl_st_init_table(vm, &sl_id_hash_type);
    klass->class_variables = sl_st_init_table(vm, &sl_id_hash_type);
    klass->instance_methods = sl_st_init_table(vm, &sl_id_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = sl_st_init_table(vm, &sl_id_hash_type);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other);

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other);

static SLVAL
sl_object_hash(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_object_is_a(sl_vm_t* vm, SLVAL self, SLVAL klass);

static SLVAL
sl_object_method(sl_vm_t* vm, SLVAL self, SLVAL method_name);

static SLVAL
sl_object_own_method(sl_vm_t* vm, SLVAL self, SLVAL method_name);

static SLVAL
sl_object_own_methods(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_object_methods(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_responds_to_slval(sl_vm_t* vm, SLVAL object, SLVAL idv);

static SLVAL
sl_object_get_instance_variable(sl_vm_t* vm, SLVAL object, SLVAL idv);

static SLVAL
sl_object_set_instance_variable(sl_vm_t* vm, SLVAL object, SLVAL idv, SLVAL value);

void
sl_init_object(sl_vm_t* vm)
{
    sl_class_t* objectp = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    sl_st_insert(objectp->constants, (sl_st_data_t)sl_intern(vm, "Object").id, (sl_st_data_t)sl_get_ptr(vm->lib.Object));
    sl_define_method(vm, vm->lib.Object, "to_s", 0, sl_object_to_s);
    sl_define_method(vm, vm->lib.Object, "inspect", 0, sl_object_inspect);
    sl_define_method(vm, vm->lib.Object, "send", -2, sl_object_send);
    sl_define_method(vm, vm->lib.Object, "responds_to", 1, sl_responds_to_slval);
    sl_define_method(vm, vm->lib.Object, "class", 0, sl_class_of);
    sl_define_method(vm, vm->lib.Object, "singleton_class", 0, sl_singleton_class);
    sl_define_method(vm, vm->lib.Object, "is_a", 1, sl_object_is_a);
    sl_define_method(vm, vm->lib.Object, "is_an", 1, sl_object_is_a);
    sl_define_method(vm, vm->lib.Object, "hash", 0, sl_object_hash);
    sl_define_method(vm, vm->lib.Object, "method", 1, sl_object_method);
    sl_define_method(vm, vm->lib.Object, "own_method", 1, sl_object_own_method);
    sl_define_method(vm, vm->lib.Object, "own_methods", 0, sl_object_own_methods);
    sl_define_method(vm, vm->lib.Object, "methods", 0, sl_object_methods);
    sl_define_method(vm, vm->lib.Object, "get_instance_variable", 1, sl_object_get_instance_variable);
    sl_define_method(vm, vm->lib.Object, "set_instance_variable", 2, sl_object_set_instance_variable);
    sl_define_method(vm, vm->lib.Object, "==", 1, sl_object_eq);
    sl_define_method(vm, vm->lib.Object, "!=", 1, sl_object_ne);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLID mid = sl_intern2(vm, argv[0]);
    return sl_send2(vm, self, mid, argc - 1, argv + 1);
}

int
sl_eq(sl_vm_t* vm, SLVAL a, SLVAL b)
{
    return sl_is_truthy(sl_send_id(vm, a, vm->id.op_eq, 1, b));
}

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    return sl_make_bool(vm, sl_get_ptr(self) == sl_get_ptr(other));
}

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    SLVAL value = sl_send_id(vm, self, vm->id.op_eq, 1, other);
    return sl_make_bool(vm, !sl_is_truthy(value));
}

static SLVAL
sl_object_hash(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, (int)(intptr_t)sl_get_ptr(self));
}

static SLVAL
sl_object_is_a(sl_vm_t* vm, SLVAL self, SLVAL klass)
{
    return sl_make_bool(vm, sl_is_a(vm, self, klass));
}

SLVAL
sl_to_s(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send_id(vm, obj, vm->id.to_s, 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_inspect(vm, obj);
    }
}

SLVAL
sl_to_s_no_throw(sl_vm_t* vm, SLVAL obj)
{
    sl_vm_frame_t frame;
    SLVAL ret, err;
    SL_TRY(frame, SL_UNWIND_EXCEPTION, {
        ret = sl_to_s(vm, obj);
    }, err, {
        (void)err;
        ret = sl_object_inspect(vm, obj);
    });
    return ret;
}

SLVAL
sl_inspect(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send_id(vm, obj, vm->id.inspect, 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_inspect(vm, obj);
    }
}

char*
sl_to_cstr(sl_vm_t* vm, SLVAL obj)
{
    SLVAL str = sl_to_s(vm, obj);
    sl_string_t* strp = (sl_string_t*)sl_get_ptr(str);
    char* buff = sl_alloc_buffer(vm->arena,strp->buff_len + 1);
    memcpy(buff, strp->buff, strp->buff_len);
    buff[strp->buff_len] = 0;
    return buff;
}

SLVAL
sl_object_to_s(sl_vm_t* vm, SLVAL self)
{
    return sl_send_id(vm, self, vm->id.inspect, 0);
}

SLVAL
sl_object_inspect(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = sl_class_of(vm, self);
    return sl_make_formatted_string(vm, "#<%V:%p>", klass, sl_get_ptr(self));
}

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL object, char* name, int arity, SLVAL(*func)())
{
    sl_define_singleton_method2(vm, object, sl_intern(vm, name), arity, func);
}

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL object, SLID name, int arity, SLVAL(*func)())
{
    SLVAL method = sl_make_c_func(vm, sl_class_of(vm, object), name, arity, func);
    sl_define_singleton_method3(vm, object, name, method);
}

void
sl_define_singleton_method3(sl_vm_t* vm, SLVAL object, SLID name, SLVAL method)
{
    sl_expect(vm, method, vm->lib.Method);
    if(sl_is_a(vm, object, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't define singleton method on Int object");
    }
    SLVAL singleton_class = sl_singleton_class(vm, object);
    sl_define_method3(vm, singleton_class, name, method);
}

static SLVAL
sl_responds_to_slval(sl_vm_t* vm, SLVAL object, SLVAL idv)
{
    return sl_make_bool(vm, sl_responds_to2(vm, object, sl_intern2(vm, idv)));
}

SLVAL
sl_get_ivar(sl_vm_t* vm, SLVAL object, SLID id)
{
    sl_object_t* p;
    SLVAL val;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        return vm->lib.nil;
    }
    p = sl_get_ptr(object);
    if(p->instance_variables && sl_st_lookup(p->instance_variables, (sl_st_data_t)id.id, (sl_st_data_t*)&val)) {
        return val;
    }
    return vm->lib.nil;
}

SLVAL
sl_get_cvar(sl_vm_t* vm, SLVAL object, SLID id)
{
    SLVAL val;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        object = sl_class_of(vm, object);
    }
    while(sl_get_primitive_type(object) == SL_T_CLASS) {
        sl_class_t* p = (sl_class_t*)sl_get_ptr(object);
        if(sl_st_lookup(p->class_variables, (sl_st_data_t)id.id, (sl_st_data_t*)&val)) {
            return val;
        }
        object = p->super;
    }
    return vm->lib.nil;
}

void
sl_set_ivar(sl_vm_t* vm, SLVAL object, SLID id, SLVAL val)
{
    sl_object_t* p;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't set instance variable on Int");
    }
    p = sl_get_ptr(object);
    if(!p->instance_variables) {
        p->instance_variables = sl_st_init_table(vm, &sl_id_hash_type);
    }
    sl_st_insert(p->instance_variables, (sl_st_data_t)id.id, (sl_st_data_t)sl_get_ptr(val));
}

void
sl_set_cvar(sl_vm_t* vm, SLVAL object, SLID id, SLVAL val)
{
    sl_class_t* p;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        object = sl_class_of(vm, object);
    }
    p = (sl_class_t*)sl_get_ptr(object);
    sl_st_insert(p->class_variables, (sl_st_data_t)id.id, (sl_st_data_t)sl_get_ptr(val));
}

static SLVAL
sl_object_get_instance_variable(sl_vm_t* vm, SLVAL object, SLVAL idv)
{
    return sl_get_ivar(vm, object, sl_intern2(vm, idv));
}

static SLVAL
sl_object_set_instance_variable(sl_vm_t* vm, SLVAL object, SLVAL idv, SLVAL value)
{
    sl_set_ivar(vm, object, sl_intern2(vm, idv), value);
    return object;
}

static SLVAL
sl_object_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    SLID mid = sl_intern2(vm, method_name);
    sl_method_t* method = sl_lookup_method(vm, self, mid);
    if(method) {
        return sl_method_bind(vm, sl_make_ptr((sl_object_t*)method), self);
    } else {
        return vm->lib.nil;
    }
}

static SLVAL
sl_object_own_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    SLID mid = sl_intern2(vm, method_name);
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;
    SLVAL method;
    sl_class_t* klassp;

    do {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        if(sl_st_lookup(klassp->instance_methods, (sl_st_data_t)mid.id, (sl_st_data_t*)&method)) {
            if(sl_get_ptr(method)->primitive_type != SL_T_CACHED_METHOD_ENTRY) {
                return method;
            }
        }
        klass = klassp->super;
    } while(klassp->singleton);

    return vm->lib.nil;
}

static int
collect_methods_iter(sl_vm_t* vm, SLID id, SLVAL method, SLVAL ary)
{
    if(sl_get_primitive_type(method) != SL_T_CACHED_METHOD_ENTRY) {
        SLVAL name = sl_id_to_string(vm, id);
        sl_array_push(vm, ary, 1, &name);
    }
    return SL_ST_CONTINUE;
}

static SLVAL
sl_object_own_methods(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;
    sl_class_t* klassp;

    SLVAL ary = sl_make_array(vm, 0, NULL);

    do {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        sl_st_foreach(klassp->instance_methods, collect_methods_iter, (sl_st_data_t)ary.i);
        klass = klassp->super;
    } while(klassp->singleton);

    return ary;
}

static SLVAL
sl_object_methods(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;

    SLVAL ary = sl_make_array(vm, 0, NULL);

    while(sl_get_primitive_type(klass) == SL_T_CLASS) {
        sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
        sl_st_foreach(klassp->instance_methods, collect_methods_iter, (sl_st_data_t)ary.i);
        klass = klassp->super;
    }

    return ary;
}

SLVAL
sl_singleton_class(sl_vm_t* vm, SLVAL object)
{
    if(sl_get_primitive_type(object) == SL_T_INT) {
        sl_throw_message2(vm, vm->lib.TypeError, "can't get singleton class for Int");
    }
    sl_object_t* ptr = sl_get_ptr(object);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(ptr->klass);
    if(klass->singleton) {
        return ptr->klass;
    }
    SLVAL singleton_class = sl_make_class(vm, ptr->klass);
    ((sl_class_t*)sl_get_ptr(singleton_class))->singleton = true;
    return ptr->klass = singleton_class;
}
