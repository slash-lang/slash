#ifndef SL_LIB_FLOAT_H
#define SL_LIB_FLOAT_H

#include "vm.h"

SLVAL
sl_make_float(sl_vm_t* vm, double n);

double
sl_get_float(sl_vm_t* vm, SLVAL f);

#endif