#include <unistd.h>

static SLVAL
posix_fork(sl_vm_t* vm, SLVAL self)
{
    int ret;
    ret = fork();
    return sl_make_int(vm, ret);
    (void)self;
}

inline static void
sl_init_ext_posix_fork(sl_vm_t* vm, SLVAL Posix)
{
    sl_define_singleton_method(vm, Posix, "fork", 0, posix_fork);
}
