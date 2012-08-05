#ifndef SL_LIB_RANGE_H
#define SL_LIB_RANGE_H

#include "value.h"
#include "vm.h"

SLVAL
sl_make_range(sl_vm_t* vm, SLVAL lower, SLVAL upper);

SLVAL
sl_make_range_exclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper);

#endif
