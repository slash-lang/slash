#ifndef SL_LIB_DICT_H
#define SL_LIB_DICT_H

#include "slash.h"

SLVAL
sl_make_dict(sl_vm_t* vm, size_t count, SLVAL* kvs);

SLVAL
sl_dict_get(sl_vm_t* vm, SLVAL dict, SLVAL key);

SLVAL
sl_dict_set(sl_vm_t* vm, SLVAL dict, SLVAL key, SLVAL val);

SLVAL
sl_dict_length(sl_vm_t* vm, SLVAL dict);

SLVAL
sl_dict_delete(sl_vm_t* vm, SLVAL dict, SLVAL key);

#endif
