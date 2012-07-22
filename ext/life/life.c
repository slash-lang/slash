#include "slash.h"

void
sl_init_ext_life(sl_vm_t* vm)
{
    SLVAL Life = sl_define_class(vm, "Life", vm->lib.Object);
    sl_class_set_const(vm, Life, "Answer", sl_make_int(vm, 42));
}