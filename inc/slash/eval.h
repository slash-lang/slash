#ifndef SL_EVAL_H
#define SL_EVAL_H

#include "vm.h"

SLVAL
sl_do_file(sl_vm_t* vm, char* filename);

SLVAL
sl_do_string(sl_vm_t* vm, uint8_t* buff, size_t len, char* filename, int start_in_slash);

#endif
