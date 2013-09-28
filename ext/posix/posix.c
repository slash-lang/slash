#include <slash.h>
#include "errno.c.inc"
#include "fork.c.inc"

void
sl_init_ext_posix(sl_vm_t* vm)
{
    SLVAL Posix = sl_define_class(vm, "Posix", vm->lib.Object);
    sl_init_ext_posix_errno(vm, Posix);
    sl_init_ext_posix_fork(vm, Posix);
}
