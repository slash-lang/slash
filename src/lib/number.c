#include "class.h"
#include "value.h"

void
sl_init_number(sl_vm_t* vm)
{
    vm->lib.Number = sl_define_class(vm, "Number", vm->lib.Object);
}
