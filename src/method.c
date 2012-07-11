#include <gc.h>
#include "class.h"
#include "method.h"

static sl_object_t*
allocate_method()
{
    return (sl_object_t*)GC_MALLOC(sizeof(sl_method_t));
}

static sl_object_t*
allocate_bound_method()
{
    return (sl_object_t*)GC_MALLOC(sizeof(sl_bound_method_t));
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
sl_make_c_func(sl_vm_t* vm, SLVAL name, int arity, SLVAL(*c_func)())
{
    SLVAL method = sl_allocate(vm, vm->lib.Method);
    sl_method_t* methp = (sl_method_t*)sl_get_ptr(method);
    methp->name = sl_expect(vm, name, vm->lib.String);
    methp->is_c_func = 1;
    methp->arity = arity;
    methp->as.c.func = c_func;
    return method;
}