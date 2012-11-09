#include <slash.h>

static SLVAL
GC_run(sl_vm_t* vm)
{
    sl_gc_run(vm->arena);
    return vm->lib.nil;
}

static SLVAL
GC_enable(sl_vm_t* vm)
{
    sl_gc_enable(vm->arena);
    return vm->lib.nil;
}

static SLVAL
GC_disable(sl_vm_t* vm)
{
    sl_gc_disable(vm->arena);
    return vm->lib.nil;
}

void
sl_init_gc(sl_vm_t* vm)
{
    SLVAL GC = sl_define_class(vm, "GC", vm->lib.Object);
    sl_define_singleton_method(vm, GC, "run", 0, GC_run);
    sl_define_singleton_method(vm, GC, "enable", 0, GC_enable);
    sl_define_singleton_method(vm, GC, "disable", 0, GC_disable);
}