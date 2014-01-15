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
#include <slash/lib/lambda.h>

static sl_object_t*
allocate_class(sl_vm_t* vm)
{
    sl_class_t* klass = sl_alloc(vm->arena, sizeof(sl_class_t));
    klass->extra = sl_alloc(vm->arena, sizeof(*klass->extra));
    klass->base.primitive_type = SL_T_CLASS;
    klass->constants = sl_st_init_table(vm, &sl_id_hash_type);
    klass->extra->class_variables = sl_st_init_table(vm, &sl_id_hash_type);
    klass->instance_methods = sl_st_init_table(vm, &sl_id_hash_type);
    klass->super = vm->lib.Object;
    klass->extra->name.id = 0;
    klass->extra->in = vm->lib.Object;
    klass->extra->doc = vm->lib.nil;
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
    sl_class_t* obj = (sl_class_t*)allocate_class(vm);
    vm->lib.Class = sl_make_ptr((sl_object_t*)obj);
    obj->extra->allocator = allocate_class;
    obj->base.klass = vm->lib.Class;
}

static SLVAL
sl_class_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_class_t* klass = get_class(vm, self);
    sl_class_t* object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    if(klass == object || sl_get_ptr(klass->extra->in) == (sl_object_t*)object) {
        return sl_id_to_string(vm, klass->extra->name);
    } else {
        return sl_make_formatted_string(vm, "%V::%I", sl_class_to_s(vm, klass->extra->in), klass->extra->name);
    }
}

static SLVAL
sl_class_name(sl_vm_t* vm, SLVAL self)
{
    return sl_id_to_string(vm, get_class(vm, self)->extra->name);
}

static SLVAL
sl_class_in(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->extra->in;
}

SLVAL
sl_class_doc(sl_vm_t* vm, SLVAL self)
{
    return get_class(vm, self)->extra->doc;
}

SLVAL
sl_class_doc_set(sl_vm_t* vm, SLVAL self, SLVAL doc)
{
    return get_class(vm, self)->extra->doc = doc;
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
    if(sl_st_lookup(klass->instance_methods, (sl_st_data_t)mid.id, (sl_st_data_t*)&method)) {
        return method;
    }
    if(sl_get_primitive_type(klass->super) == SL_T_CLASS) {
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
    if(sl_st_lookup(klass->instance_methods, (sl_st_data_t)mid.id, (sl_st_data_t*)&method)) {
        return method;
    }
    return vm->lib.nil;
}

struct own_instance_methods_iter_state {
    sl_vm_t* vm;
    SLVAL ary;
};

static int
own_instance_methods_iter(sl_vm_t* vm, SLID name, SLVAL method, SLVAL ary)
{
    SLVAL namev = sl_id_to_string(vm, name);
    sl_array_push(vm, ary, 1, &namev);
    return SL_ST_CONTINUE;
    (void)method;
}

SLVAL
sl_class_own_instance_methods(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* class = get_class(vm, klass);
    SLVAL ary = sl_make_array(vm, 0, NULL);
    sl_st_foreach(class->instance_methods, own_instance_methods_iter, (sl_st_data_t)ary.i);
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
class_constants_iter(sl_vm_t* vm, SLID id, SLVAL value, SLVAL ary)
{
    SLVAL name = sl_id_to_string(vm, id);
    sl_array_push(vm, ary, 1, &name);
    return SL_ST_CONTINUE;
    (void)value;
}

static SLVAL
class_constants(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* class = get_class(vm, klass);
    SLVAL ary = sl_make_array(vm, 0, NULL);
    sl_st_foreach(class->constants, class_constants_iter, (sl_st_data_t)ary.i);
    return ary;
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
    sl_st_delete(class->constants, (sl_st_data_t*)&id.id, NULL);
    vm->state_constant++;
    return klass;
}

static SLVAL
class_singleton(sl_vm_t* vm, SLVAL klass)
{
    return sl_make_bool(vm, get_class(vm, klass)->base.user_flags & SL_FLAG_CLASS_SINGLETON);
}

static SLVAL
class_define_method(sl_vm_t* vm, SLVAL klass, SLVAL name, SLVAL lambda)
{
    SLVAL method = sl_lambda_to_method(vm, lambda);
    sl_define_method3(vm, klass, sl_intern2(vm, name), method);
    return method;
}

static SLVAL
class_has_instance(sl_vm_t* vm, SLVAL klass, SLVAL object)
{
    return sl_make_bool(vm, sl_is_a(vm, object, klass));
}

static SLVAL
class_subclassed(sl_vm_t* vm)
{
    return vm->lib.nil;
}

void
sl_init_class(sl_vm_t* vm)
{
    sl_class_t* objectp = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    SLID cid = sl_intern(vm, "Class");
    sl_st_insert(objectp->constants, (sl_st_data_t)cid.id, (sl_st_data_t)sl_get_ptr(vm->lib.Class));

    sl_define_method(vm, vm->lib.Class, "to_s", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "name", 0, sl_class_name);
    sl_define_method(vm, vm->lib.Class, "in", 0, sl_class_in);
    sl_define_method(vm, vm->lib.Class, "doc", 0, sl_class_doc);
    sl_define_method(vm, vm->lib.Class, "super", 0, sl_class_super);
    sl_define_method(vm, vm->lib.Class, "inspect", 0, sl_class_to_s);
    sl_define_method(vm, vm->lib.Class, "new", -1, sl_new);
    sl_define_method(vm, vm->lib.Class, "file_path", 0, sl_class_file_path);
    sl_define_method(vm, vm->lib.Class, "instance_method", 1, sl_class_instance_method);
    sl_define_method(vm, vm->lib.Class, "own_instance_method", 1, sl_class_own_instance_method);
    sl_define_method(vm, vm->lib.Class, "own_instance_methods", 0, sl_class_own_instance_methods);
    sl_define_method(vm, vm->lib.Class, "instance_methods", 0, sl_class_instance_methods);
    sl_define_method(vm, vm->lib.Class, "define_method", 2, class_define_method);
    sl_define_method(vm, vm->lib.Class, "constants", 0, class_constants);
    sl_define_method(vm, vm->lib.Class, "get_constant", 1, class_get_constant);
    sl_define_method(vm, vm->lib.Class, "set_constant", 2, class_set_constant);
    sl_define_method(vm, vm->lib.Class, "remove_constant", 1, class_remove_constant);
    sl_define_method(vm, vm->lib.Class, "singleton", 0, class_singleton);
    sl_define_method(vm, vm->lib.Class, "has_instance", 1, class_has_instance);
    sl_define_method(vm, vm->lib.Class, "subclassed", 1, class_subclassed);
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
    klass->extra->name = name;
    klass->extra->in = sl_expect(vm, in, vm->lib.Class);
    sl_st_insert(((sl_class_t*)sl_get_ptr(in))->constants,
        (sl_st_data_t)name.id, (sl_st_data_t)klass);
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

    sing->extra->allocator = NULL;
    sing->super = super->base.klass;
    sing->base.user_flags |= SL_FLAG_CLASS_SINGLETON;

    klass->base.klass = vsing;
    klass->extra->allocator = super->extra->allocator;
    klass->super = vsuper;

    if(!vm->initializing) {
        sl_send_id(vm, vsuper, vm->id.subclassed, 1, vklass);
    }

    return vklass;
}

void
sl_class_set_allocator(sl_vm_t* vm, SLVAL klass, sl_object_t*(*allocator)(sl_vm_t*))
{
    get_class(vm, klass)->extra->allocator = allocator;
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
    sl_st_insert(get_class(vm, klass)->instance_methods, (sl_st_data_t)name.id, (sl_st_data_t)sl_get_ptr(method));
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
    return sl_st_lookup(klassp->constants, (sl_st_data_t)name.id, NULL);
}

int
sl_class_has_const2(sl_vm_t* vm, SLVAL klass, SLID name)
{
    sl_class_t* klassp;
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    if(sl_st_lookup(klassp->constants, (sl_st_data_t)name.id, NULL)) {
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
    if(!sl_is_a(vm, klass, vm->lib.Class)) {
        klass = sl_class_of(vm, klass);
    }
    klassp = get_class(vm, klass);
    SLVAL val;
    if(sl_st_lookup(klassp->constants, (sl_st_data_t)name.id, (sl_st_data_t*)&val)) {
        return val;
    }
    if(sl_get_primitive_type(klassp->super) == SL_T_CLASS && sl_class_has_const2(vm, klassp->super, name)) {
        return sl_class_get_const2(vm, klassp->super, name);
    }
    sl_error(vm, vm->lib.NameError, "Undefined constant %QI in %V", name, klass);
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
    if(sl_class_has_own_const(vm, klass, name)) {
        sl_error(vm, vm->lib.NameError, "Constant %QI in %V already defined", name, klass);
    }
    klassp = get_class(vm, klass);
    sl_expect(vm, klass, vm->lib.Class);
    sl_st_insert(klassp->constants, (sl_st_data_t)name.id, (sl_st_data_t)sl_get_ptr(val));
    vm->state_constant++;
}

int
sl_is_a(sl_vm_t* vm, SLVAL obj, SLVAL klass)
{
    SLVAL vk = sl_real_class_of(vm, obj);
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
sl_real_class_of(sl_vm_t* vm, SLVAL obj)
{
    if(sl_get_primitive_type(obj) == SL_T_INT) {
        return vm->lib.Int;
    } else {
        return sl_get_ptr(obj)->klass;
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
        while(pklass->base.user_flags & SL_FLAG_CLASS_SINGLETON) {
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
    } else if(argc) {
        sl_error(vm, vm->lib.ArgumentError, "Too many arguments. Expected 0, received %d.", argc);
    }
    return obj;
}

bool
sl_class_has_full_path(sl_vm_t* vm, SLVAL klass)
{
    if(sl_get_ptr(klass) == sl_get_ptr(vm->lib.Object)) {
        return true;
    }
    if(sl_get_primitive_type(klass) != SL_T_CLASS) {
        return false;
    }
    sl_class_t* klassp = get_class(vm, klass);
    if(!klassp->extra->name.id) {
        return false;
    }
    return sl_class_has_full_path(vm, klassp->extra->in);
}

SLVAL
sl_camel_case_to_underscore(sl_vm_t* vm, SLVAL strv)
{
    sl_string_t* str = sl_get_string(vm, strv);
    size_t len = str->buff_len;
    bool saw_capital_letter = true;
    for(size_t i = 0; i < str->buff_len; i++) {
        uint8_t c = str->buff[i];
        if(c >= 'A' && c <= 'Z') {
            if(!saw_capital_letter) {
                len++;
                saw_capital_letter = true;
            }
        } else {
            saw_capital_letter = false;
        }
    }
    uint8_t* file_path = sl_alloc_buffer(vm->arena, len + 1);
    size_t j = 0;
    saw_capital_letter = true;
    for(size_t i = 0; i < str->buff_len; i++) {
        uint8_t c = str->buff[i];
        if(c >= 'A' && c <= 'Z') {
            if(!saw_capital_letter) {
                file_path[j++] = '_';
                saw_capital_letter = true;
            }
            file_path[j++] = c + ('a' - 'A');
        } else {
            saw_capital_letter = false;
            file_path[j++] = c;
        }
    }
    return sl_make_string(vm, file_path, j);
}

static SLVAL
class_file_path_rec(sl_vm_t* vm, SLVAL klass)
{
    sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
    SLVAL seg = sl_camel_case_to_underscore(vm, sl_id_to_string(vm, klassp->extra->name));
    if(sl_get_ptr(klassp->extra->in) == sl_get_ptr(vm->lib.Object)) {
        return seg;
    }
    SLVAL path = class_file_path_rec(vm, klassp->extra->in);
    path = sl_string_concat(vm, path, sl_make_cstring(vm, "/"));
    return sl_string_concat(vm, path, seg);
}

SLVAL
sl_class_file_path(sl_vm_t* vm, SLVAL klass)
{
    if(!sl_class_has_full_path(vm, klass)) {
        return vm->lib.nil;
    }
    if(sl_get_ptr(klass) == sl_get_ptr(vm->lib.Object)) {
        return sl_make_cstring(vm, "Object");
    }
    return class_file_path_rec(vm, klass);
}
