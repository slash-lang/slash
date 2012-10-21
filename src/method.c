#include <slash/class.h>
#include <slash/method.h>
#include <slash/object.h>

static sl_object_t*
allocate_method(sl_vm_t* vm)
{
    sl_object_t* method = sl_alloc(vm->arena, sizeof(sl_method_t));
    method->primitive_type = SL_T_METHOD;
    return method;
}

static sl_object_t*
allocate_bound_method(sl_vm_t* vm)
{
    sl_object_t* bound_method = sl_alloc(vm->arena, sizeof(sl_bound_method_t));
    bound_method->primitive_type = SL_T_BOUND_METHOD;
    return bound_method;
}

static SLVAL
method_apply(sl_vm_t* vm, SLVAL method, size_t argc, SLVAL* argv)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!methp->initialized) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't apply uninitialized Method");
    }
    return sl_apply_method(vm, argv[0], methp, argc - 1, argv + 1);
}

static SLVAL
sl_method_name(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!methp->initialized) {
        return vm->lib.nil;
    }
    return methp->name;
}

static SLVAL
sl_method_on(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!methp->initialized) {
        return vm->lib.nil;
    }
    return methp->klass;
}

static SLVAL
sl_method_arity(sl_vm_t* vm, SLVAL method)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    if(!methp->initialized) {
        return vm->lib.nil;
    }
    return sl_make_int(vm, methp->arity);
}

static SLVAL
bound_method_call(sl_vm_t* vm, SLVAL bmethod, size_t argc, SLVAL* argv)
{
    sl_bound_method_t* bmethp = (sl_bound_method_t*)sl_get_ptr(bmethod);
    if(!bmethp->method.initialized) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't call uninitialized BoundMethod");
    }
    return sl_apply_method(vm, bmethp->self, &bmethp->method, argc, argv);
}

static SLVAL
bound_method_unbind(sl_vm_t* vm, SLVAL bmethod)
{
    sl_bound_method_t* bmethp = (sl_bound_method_t*)sl_get_ptr(bmethod);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(sl_allocate(vm, vm->lib.Method));
    methp->name         = bmethp->method.name;
    methp->is_c_func    = bmethp->method.is_c_func;
    methp->arity        = bmethp->method.arity;
    methp->klass        = bmethp->method.klass;
    methp->as           = bmethp->method.as;
    methp->initialized  = 1;
    return sl_make_ptr((sl_object_t*)methp);
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
    
    vm->lib.BoundMethod = sl_define_class(vm, "BoundMethod", vm->lib.Method);
    sl_class_set_allocator(vm, vm->lib.BoundMethod, allocate_bound_method);
    sl_define_method(vm, vm->lib.BoundMethod, "unbind", 0, bound_method_unbind);
    sl_define_method(vm, vm->lib.BoundMethod, "call", -1, bound_method_call);
}

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*c_func)())
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->name = sl_expect(vm, name, vm->lib.String);
    methp->is_c_func = 1;
    methp->arity = arity;
    methp->klass = sl_expect(vm, klass, vm->lib.Class);
    methp->as.c.func = c_func;
    methp->initialized = 1;
    return method;
}

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLVAL name, sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx)
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->name = sl_expect(vm, name, vm->lib.String);
    methp->is_c_func = 0;
    if(section->req_registers < section->arg_registers) {
        methp->arity = -section->req_registers - 1;
    } else {
        methp->arity = (int)section->arg_registers;
    }
    methp->klass = sl_expect(vm, klass, vm->lib.Class);
    methp->as.sl.section = section;
    methp->as.sl.parent_ctx = parent_ctx;
    methp->initialized = 1;
    return method;
}

SLVAL
sl_method_bind(sl_vm_t* vm, SLVAL method, SLVAL receiver)
{
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    sl_bound_method_t* bmethp = (sl_bound_method_t*)sl_get_ptr(sl_allocate(vm, vm->lib.BoundMethod));
    
    if(!methp->initialized) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't bind uninitialized Method");
    }
    
    bmethp->method.initialized  = 1;
    bmethp->method.name         = methp->name;
    bmethp->method.klass        = methp->klass;
    bmethp->method.is_c_func    = methp->is_c_func;
    bmethp->method.arity        = methp->arity;
    bmethp->method.as           = methp->as;
    
    bmethp->self = sl_expect(vm, receiver, methp->klass);
    return sl_make_ptr((sl_object_t*)bmethp);
}
