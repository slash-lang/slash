#include <slash/value.h>
#include <slash/vm.h>
#include <slash/class.h>
#include <slash/string.h>
#include <slash/object.h>
#include <slash/lib/bignum.h>
#include <stdlib.h>
#include <time.h>

static SLVAL
object_exit(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL exit_code = sl_make_int(vm, 0);
    if(argc) {
        exit_code = argv[0];
    }
    sl_exit(vm, exit_code);
    return self; /* never reached */
}

static SLVAL
object_strftime(sl_vm_t* vm, SLVAL self, SLVAL format, SLVAL time)
{
    time_t t;
    struct tm* tm_ptr;
    size_t capacity = 256;
    char* fmt = sl_to_cstr(vm, format);
    char* buff = sl_alloc_buffer(vm->arena, capacity);
    if(sl_is_a(vm, time, vm->lib.Int)) {
        t = sl_get_int(time);
    } else if(sl_is_a(vm, time, vm->lib.Bignum)) {
        t = sl_bignum_get_long(vm, time);
    } else {
        sl_expect(vm, time, vm->lib.Int);
    }
    tm_ptr = gmtime(&t);
    while(strftime(buff, capacity, fmt, tm_ptr) == 0) {
        capacity *= 2;
        buff = sl_realloc(vm->arena, buff, capacity);
    }
    return sl_make_cstring(vm, buff);
    (void)self; /* never reached */
}

void
sl_init_system(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "exit", -1, object_exit);
    sl_define_method(vm, vm->lib.Object, "strftime", 2, object_strftime);
}
