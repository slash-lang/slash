#include <unistd.h>

static SLVAL
posix_fork(sl_vm_t* vm, SLVAL self)
{
    int ret;
    if ((ret = fork()) < 0) {
        SLVAL error = sl_new_oserror(vm, errno);
        sl_throw(vm, error);
    }
    return sl_make_int(vm, ret);

    (void)self;
}

static SLVAL
posix_fork_prime(sl_vm_t* vm, SLVAL self)
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
    sl_define_singleton_method(vm, Posix, "fork'", 0, posix_fork_prime);
}
