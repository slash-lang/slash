#ifndef SL_LIB_RAND_H
#define SL_LIB_RAND_H

#include "vm.h"

int
sl_rand(sl_vm_t* vm);

void
sl_rand_init_mt(sl_vm_t* vm);

#endif
