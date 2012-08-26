#ifndef SL_EVAL_H
#define SL_EVAL_H

#include "slash/vm.h"

SLVAL
sl_do_file(sl_vm_t* vm, uint8_t* filename);

SLVAL
sl_do_string(sl_vm_t* vm, uint8_t* buff, size_t len, uint8_t* filename);

#endif
