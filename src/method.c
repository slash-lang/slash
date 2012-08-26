#include "slash/class.h"
#include "slash/method.h"

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

void
sl_init_method(sl_vm_t* vm)
{
    vm->lib.Method = sl_define_class(vm, "Method", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Method, allocate_method);
    
    vm->lib.BoundMethod = sl_define_class(vm, "BoundMethod", vm->lib.Method);
    sl_class_set_allocator(vm, vm->lib.BoundMethod, allocate_bound_method);
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
    return method;
}

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLVAL name, sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx)
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->name = sl_expect(vm, name, vm->lib.String);
    methp->is_c_func = 0;
    methp->arity = (int)section->arg_registers;
    methp->klass = sl_expect(vm, klass, vm->lib.Class);
    methp->as.sl.section = section;
    methp->as.sl.parent_ctx = parent_ctx;
    return method;
}
