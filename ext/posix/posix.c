#include <slash.h>
#include <stdlib.h>
#include <string.h>

static int cPosix_OSError;

static SLVAL
sl_new_oserror(sl_vm_t* vm, int errno)
{
    char* description = strerror(errno);
    SLVAL Posix_OSError = sl_vm_store_get(vm, &cPosix_OSError);
    SLVAL error = sl_make_error2(vm, Posix_OSError, sl_make_cstring(vm, description));
    sl_set_ivar(vm, error, sl_intern(vm, "errno"), sl_make_int(vm, errno));
    return error;
}

static SLVAL
sl_posix_oserror_get_errno(sl_vm_t* vm, SLVAL self)
{
    return sl_get_ivar(vm, self, sl_intern(vm, "errno"));
}

#include "errno.c.inc"
#include "fork.c.inc"

void
sl_init_ext_posix(sl_vm_t* vm)
{
    SLVAL Posix = sl_define_class(vm, "Posix", vm->lib.Object);
    SLVAL Posix_OSError = sl_define_class3(vm, sl_intern(vm, "OSError"), vm->lib.Error, Posix);
    sl_define_method(vm, Posix_OSError, "errno", 0, sl_posix_oserror_get_errno);

    sl_vm_store_put(vm, &cPosix_OSError, Posix_OSError);

    sl_init_ext_posix_errno(vm, Posix);
    sl_init_ext_posix_fork(vm, Posix);
}
