#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slash/class.h>
#include <slash/method.h>
#include <slash/object.h>
#include <slash/string.h>
#include <slash/dispatch.h>

static int
id_hash(sl_vm_t* vm, SLID id)
{
    return (int)id.id;
    (void)vm;
}

static int
id_cmp(sl_vm_t* vm, SLID a, SLID b)
{
    if(a.id > b.id) {
        return 1;
    } else if(a.id < b.id) {
        return -1;
    } else {
        return 0;
    }
    (void)vm;
}

struct sl_st_hash_type
sl_id_hash_type = { id_cmp, id_hash };

static sl_object_t*
allocate_method(sl_vm_t* vm)
{
    sl_method_t* method = sl_alloc(vm->arena, sizeof(sl_method_t));
    method->extra = sl_alloc(vm->arena, sizeof(*method->extra));
    method->base.primitive_type = SL_T_METHOD;
    method->extra->doc = vm->lib.nil;
    return (sl_object_t*)method;
}

static SLVAL
method_apply(sl_vm_t* vm, SLVAL method, size_t argc, SLVAL* argv)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't apply uninitialized Method");
    }
    return sl_apply_method(vm, argv[0], methp, argc - 1, argv + 1);
}

static SLVAL
sl_method_name(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        return vm->lib.nil;
    }
    return sl_id_to_string(vm, methp->extra->name);
}

static SLVAL
sl_method_on(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        return vm->lib.nil;
    }
    return methp->extra->klass;
}

static SLVAL
sl_method_arity(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        return vm->lib.nil;
    }
    return sl_make_int(vm, methp->arity);
}

SLVAL
sl_method_doc(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        return vm->lib.nil;
    }
    return methp->extra->doc;
}

SLVAL
sl_method_doc_set(sl_vm_t* vm, SLVAL method, SLVAL doc)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    return methp->extra->doc = doc;
    (void)vm;
}

static SLVAL
bound_method_call(sl_vm_t* vm, SLVAL bmethod, size_t argc, SLVAL* argv)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(bmethod);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't call uninitialized BoundMethod");
    }
    return sl_apply_method(vm, methp->extra->bound_self, methp, argc, argv);
}

static sl_method_t*
method_dup(sl_vm_t* vm, sl_method_t* method)
{
    sl_method_t* new_method = sl_alloc(vm->arena, sizeof(sl_method_t));
    memcpy(new_method, method, sizeof(*method));
    new_method->extra = sl_alloc(vm->arena, sizeof(*new_method->extra));
    memcpy(new_method->extra, method->extra, sizeof(*method->extra));
    return new_method;
}

static SLVAL
bound_method_unbind(sl_vm_t* vm, SLVAL bmethod)
{
    sl_method_t* bmethp = (sl_method_t*)sl_get_ptr(bmethod);
    if(!(bmethp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't unbind uninitalized BoundMethod");
    }
    sl_method_t* methp = method_dup(vm, bmethp);
    methp->extra->bound_self = vm->lib.nil;
    methp->base.klass = vm->lib.Method;
    return sl_make_ptr((sl_object_t*)methp);
}

static SLVAL
method_inspect(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        return sl_object_inspect(vm, method);
    }

    return sl_make_formatted_string(vm, "#<Method: %V#%I(%d)>",
        methp->extra->klass, methp->extra->name, methp->arity);
}

static SLVAL
method_eq(sl_vm_t* vm, SLVAL method, SLVAL other)
{
    if(sl_get_ptr(sl_class_of(vm, other)) != sl_get_ptr(vm->lib.Method)) {
        return vm->lib._false;
    }
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    sl_method_t* othp = (sl_method_t*)sl_get_ptr(other);
    return sl_make_bool(vm, memcmp(methp, othp, sizeof(sl_method_t)) == 0);
}

static SLVAL
bound_method_eq(sl_vm_t* vm, SLVAL method, SLVAL other)
{
    if(sl_get_ptr(sl_class_of(vm, other)) != sl_get_ptr(vm->lib.BoundMethod)) {
        return vm->lib._false;
    }
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    sl_method_t* othp = (sl_method_t*)sl_get_ptr(other);
    if(memcmp(methp, othp, sizeof(sl_method_t)) != 0) {
        return vm->lib._false;
    }
    if(memcmp(methp->extra, othp->extra, sizeof(*methp->extra)) != 0) {
        return vm->lib._false;
    }
    return vm->lib._true;
}

void
sl_init_method(sl_vm_t* vm)
{
    vm->lib.Method = sl_define_class(vm, "Method", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Method, allocate_method);
    sl_define_method(vm, vm->lib.Method, "bind", 1, sl_method_bind);
    sl_define_method(vm, vm->lib.Method, "apply", -2, method_apply);
    sl_define_method(vm, vm->lib.Method, "name", 0, sl_method_name);
    sl_define_method(vm, vm->lib.Method, "on", 0, sl_method_on);
    sl_define_method(vm, vm->lib.Method, "arity", 0, sl_method_arity);
    sl_define_method(vm, vm->lib.Method, "doc", 0, sl_method_doc);
    sl_define_method(vm, vm->lib.Method, "inspect", 0, method_inspect);
    sl_define_method(vm, vm->lib.Method, "==", 1, method_eq);
    
    vm->lib.BoundMethod = sl_define_class(vm, "BoundMethod", vm->lib.Method);
    sl_define_method(vm, vm->lib.BoundMethod, "unbind", 0, bound_method_unbind);
    sl_define_method(vm, vm->lib.BoundMethod, "call", -1, bound_method_call);
    sl_define_method(vm, vm->lib.BoundMethod, "==", 1, bound_method_eq);
}

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL klass, SLID name, int arity, SLVAL(*c_func)())
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->extra->name = name;
    methp->arity = arity;
    methp->extra->klass = sl_expect(vm, klass, vm->lib.Class);
    methp->as.c.func = c_func;
    methp->base.user_flags |= SL_FLAG_METHOD_INITIALIZED
                            | SL_FLAG_METHOD_IS_C_FUNC;
    return method;
}

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLID name, sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx)
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->extra->name = name;
    if(section->req_registers < section->arg_registers) {
        methp->arity = -section->req_registers - 1;
    } else if(section->has_extra_rest_arg) {
        methp->arity = -(int)section->req_registers - 1;
    } else {
        methp->arity = (int)section->req_registers;
    }
    methp->extra->klass = sl_expect(vm, klass, vm->lib.Class);
    methp->as.sl.section = section;
    methp->as.sl.parent_ctx = parent_ctx;
    methp->base.user_flags |= SL_FLAG_METHOD_INITIALIZED;
    return method;
}

SLVAL
sl_method_bind(sl_vm_t* vm, SLVAL method, SLVAL receiver)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!(methp->base.user_flags & SL_FLAG_METHOD_INITIALIZED)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't bind uninitialized Method");
    }
    sl_method_t* bmethp = method_dup(vm, methp);
    bmethp->extra->bound_self = sl_expect(vm, receiver, methp->extra->klass);
    bmethp->base.klass = vm->lib.BoundMethod;
    return sl_make_ptr((sl_object_t*)bmethp);
}

SLVAL
sl_id_to_string(sl_vm_t* vm, SLID id)
{
    if(id.id >= vm->intern.id_to_name_size) {
        abort();
    }
    return vm->intern.id_to_name[id.id];
}

static SLID
sl_intern2_no_check(sl_vm_t* vm, SLVAL str)
{
    SLID id;

    if(sl_st_lookup(vm->intern.name_to_id, (sl_st_data_t)sl_get_ptr(str), (sl_st_data_t*)&id)) {
        return id;
    }

    id.id = vm->intern.id_to_name_size++;
    sl_st_insert(vm->intern.name_to_id, (sl_st_data_t)sl_get_ptr(str), (sl_st_data_t)id.id);
    if(vm->intern.id_to_name_size >= vm->intern.id_to_name_cap) {
        vm->intern.id_to_name_cap *= 2;
        vm->intern.id_to_name = sl_realloc(vm->arena, vm->intern.id_to_name, sizeof(SLVAL) * vm->intern.id_to_name_cap);
    }
    vm->intern.id_to_name[id.id] = str;

    return id;
}

SLID
sl_intern2(sl_vm_t* vm, SLVAL str)
{
    return sl_intern2_no_check(vm, sl_expect(vm, str, vm->lib.String));
}

SLID
sl_intern(sl_vm_t* vm, char* cstr)
{
    return sl_intern2(vm, sl_make_cstring(vm, cstr));
}

SLID
sl_id_make_setter(sl_vm_t* vm, SLID id)
{
    SLVAL str = sl_make_formatted_string(vm, "%I=", id);
    return sl_intern2(vm, str);
}
