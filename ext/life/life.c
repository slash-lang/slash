#include "slash.h"

static int
meaning_of_life = -1;

void
sl_static_init_ext_life()
{
    meaning_of_life = 42;
}

void
sl_init_ext_life(sl_vm_t* vm)
{
    SLVAL Life = sl_define_class(vm, "Life", vm->lib.Object);
    sl_class_set_const(vm, Life, "Answer", sl_make_int(vm, meaning_of_life));
}