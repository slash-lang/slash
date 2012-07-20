#include <stdio.h>
#include "slash.h"

static SLVAL
io_print(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    size_t i;
    sl_string_t* str;
    (void)self;
    for(i = 0; i < argc; i++) {
        str = (sl_string_t*)sl_get_ptr(sl_to_s(vm, argv[i]));
        vm->output(vm, (char*)str->buff, str->buff_len);
    }
    return vm->lib.nil;
}

void
sl_init_io(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "print", -1, io_print);
}
