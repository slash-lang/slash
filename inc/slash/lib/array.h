#ifndef SL_LIB_ARRAY_H
#define SL_LIB_ARRAY_H

#include <slash.h>

SLVAL
sl_make_array(sl_vm_t* vm, size_t count, SLVAL* items);

SLVAL
sl_array_get(sl_vm_t* vm, SLVAL array, int i);

SLVAL
sl_array_get2(sl_vm_t* vm, SLVAL array, SLVAL i);

SLVAL
sl_array_set(sl_vm_t* vm, SLVAL array, int i, SLVAL val);

SLVAL
sl_array_set2(sl_vm_t* vm, SLVAL array, SLVAL i, SLVAL val);

SLVAL
sl_array_length(sl_vm_t* vm, SLVAL array);

SLVAL
sl_array_push(sl_vm_t* vm, SLVAL array, size_t count, SLVAL* items);

SLVAL
sl_array_pop(sl_vm_t* vm, SLVAL array);

SLVAL
sl_array_unshift(sl_vm_t* vm, SLVAL array, size_t count, SLVAL* items);

SLVAL
sl_array_shift(sl_vm_t* vm, SLVAL array);

SLVAL
sl_array_sort(sl_vm_t* vm, SLVAL array, size_t argc, SLVAL* argv);

size_t
sl_array_items(sl_vm_t* vm, SLVAL array, SLVAL** items);

size_t
sl_array_items_no_copy(sl_vm_t* vm, SLVAL array, SLVAL** items);

SLVAL
sl_array_concat(sl_vm_t* vm, SLVAL array, SLVAL other);

SLVAL
sl_array_diff(sl_vm_t* vm, SLVAL array, SLVAL other);

SLVAL
sl_array_join(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* joiner);

#endif
