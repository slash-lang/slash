#include <unistd.h>

static SLVAL
posix_getpid(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, getpid());
    (void)self;
}

static SLVAL
posix_getppid(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, getppid());
    (void)self;
}

inline static void
sl_init_ext_posix_getpid(sl_vm_t* vm, SLVAL Posix)
{
    sl_define_singleton_method(vm, Posix, "getpid", 0, posix_getpid);
    sl_define_singleton_method(vm, Posix, "getppid", 0, posix_getppid);
}
