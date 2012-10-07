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

void
sl_init_system(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "exit", -1, object_exit);
}
