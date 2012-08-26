#ifndef SL_UTF8_H
#define SL_UTF8_H

#include "slash/vm.h"
#include <stddef.h>

size_t
sl_utf8_strlen(sl_vm_t* vm, uint8_t* buff, size_t len);

uint32_t*
sl_utf8_to_utf32(sl_vm_t* vm, uint8_t* buff, size_t len, size_t* out_len);

uint32_t
sl_utf8_each_char(sl_vm_t* vm, uint8_t** buff_ptr, size_t* len);

size_t
sl_utf32_char_to_utf8(sl_vm_t* vm, uint32_t u32c, uint8_t* u8buff);

uint8_t*
sl_utf32_to_utf8(sl_vm_t* vm, uint32_t* u32, size_t u32_len, size_t* buff_len);

#endif
