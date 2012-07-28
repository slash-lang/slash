#ifndef SL_LIB_ENUMERABLE_H
#define SL_LIB_ENUMERABLE_H

#include "value.h"
#include "vm.h"

SLVAL
sl_enumerable_join(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

#endif
