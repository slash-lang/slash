#include "class.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SLVAL
sl_number_parse(sl_vm_t* vm, uint8_t* str, size_t len)
{
    /* TODO handle bignum upgrade as well */
    int n;
    char* buff = alloca(len + 1);
    memcpy(buff, str, len);
    buff[len + 1] = 0;
    sscanf(buff, "%d", &n);
    return sl_make_int(vm, n);
}

void
sl_init_number(sl_vm_t* vm)
{
    vm->lib.Number = sl_define_class(vm, "Number", vm->lib.Object);
}
