#include <slash.h>

void
sl_init_version(sl_vm_t* vm)
{
    SLVAL Slash = sl_define_class(vm, "Slash", vm->lib.Object);
    sl_class_set_const(vm, Slash, "VERSION", sl_make_cstring(vm, SL_VERSION));
}
