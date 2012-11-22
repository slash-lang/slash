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

static SLVAL
GC_alloc_count(sl_vm_t* vm)
{
    return sl_make_int(vm, sl_gc_alloc_count(vm->arena));
}

static SLVAL
GC_memory_usage(sl_vm_t* vm)
{
    return sl_make_int(vm, sl_gc_memory_usage(vm->arena));
}

void
sl_init_gc(sl_vm_t* vm)
{
    SLVAL GC = sl_define_class(vm, "GC", vm->lib.Object);
    sl_define_singleton_method(vm, GC, "run", 0, GC_run);
    sl_define_singleton_method(vm, GC, "enable", 0, GC_enable);
    sl_define_singleton_method(vm, GC, "disable", 0, GC_disable);
    sl_define_singleton_method(vm, GC, "alloc_count", 0, GC_alloc_count);
    sl_define_singleton_method(vm, GC, "memory_usage", 0, GC_memory_usage);
}
