#ifndef SL_LIB_REGEXP_H
#define SL_LIB_REGEXP_H

#include "slash/vm.h"
#include "slash/value.h"

SLVAL
sl_make_regexp(sl_vm_t* vm, uint8_t* re, size_t re_len, uint8_t* opts, size_t opts_len);

#endif
