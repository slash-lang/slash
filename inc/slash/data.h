#ifndef SL_DATA_H
#define SL_DATA_H

#include <slash/vm.h>
#include <slash/value.h>

sl_object_t*
sl_make_data(sl_vm_t* vm, SLVAL klass, sl_data_type_t* type, void* data);

void*
sl_data_get_ptr(sl_vm_t* vm, sl_data_type_t* type, SLVAL object);

void
sl_data_set_ptr(sl_vm_t* vm, sl_data_type_t* type, SLVAL object, void* data);

#endif
