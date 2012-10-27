#include <slash.h>

static SLVAL
GC_run(sl_vm_t* vm)
{
    sl_gc_run(vm->arena);
    return vm->lib.nil;
}

void
sl_init_gc(sl_vm_t* vm)
{
    SLVAL GC = sl_define_class(vm, "GC", vm->lib.Object);
    sl_define_singleton_method(vm, GC, "run", 0, GC_run);
}