#ifndef SL_STRING_H
#define SL_STRING_H

#include "vm.h"

extern struct st_hash_type
sl_string_hash_type;

SLVAL
sl_make_string(struct sl_vm* vm, uint8_t* buff, size_t len);

SLVAL
sl_make_cstring(struct sl_vm* vm, char* cstr);

void
sl_init_string(sl_vm_t* vm);

#endif