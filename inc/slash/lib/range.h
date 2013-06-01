#ifndef SL_LIB_RANGE_H
#define SL_LIB_RANGE_H

#include <slash/value.h>
#include <slash/vm.h>

SLVAL
sl_make_range_inclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper);

SLVAL
sl_make_range_exclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper);

SLVAL
sl_range_lower(sl_vm_t* vm, SLVAL range);

SLVAL
sl_range_upper(sl_vm_t* vm, SLVAL range);

bool
sl_range_is_exclusive(sl_vm_t* vm, SLVAL range);

#endif
