#ifndef SL_LIB_RANGE_H
#define SL_LIB_RANGE_H

#include <slash/value.h>
#include <slash/vm.h>

SLVAL
sl_make_range(sl_vm_t* vm, SLVAL lower, SLVAL upper);

SLVAL
sl_make_range_exclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper);

#endif
