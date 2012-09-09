#ifndef SL_LIB_NUMBER_H
#define SL_LIB_NUMBER_H

#include <slash/vm.h>
#include <stdint.h>

SLVAL
sl_integer_parse(sl_vm_t* vm, uint8_t* str, size_t len);

#endif
