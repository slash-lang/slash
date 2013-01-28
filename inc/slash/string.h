#ifndef SL_STRING_H
#define SL_STRING_H

#include "vm.h"

extern struct st_hash_type
sl_string_hash_type;

SLVAL
sl_make_string(struct sl_vm* vm, uint8_t* buff, size_t buff_len);

SLVAL
sl_make_string_no_copy(struct sl_vm* vm, uint8_t* buff, size_t buff_len);

SLVAL
sl_make_string_enc(sl_vm_t* vm, char* buff, size_t buff_len, char* encoding);

SLVAL
sl_make_cstring(struct sl_vm* vm, const char* cstr);

sl_string_t*
sl_cstring(struct sl_vm* vm, char* cstr);

sl_string_t*
sl_get_string(sl_vm_t* vm, SLVAL str);

SLVAL
sl_string_length(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_concat(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_string_inspect(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_html_escape(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_url_decode(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_url_encode(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_index(sl_vm_t* vm, SLVAL self, SLVAL substr);

SLVAL
sl_string_split(sl_vm_t* vm, SLVAL self, SLVAL delim);

SLVAL
sl_string_format(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

SLVAL
sl_string_eq(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_string_encode(sl_vm_t* vm, SLVAL self, char* encoding);

SLVAL
sl_string_encode2(sl_vm_t* vm, SLVAL self, SLVAL encoding);

SLVAL
sl_string_upper(sl_vm_t* vm, SLVAL self);

SLVAL
sl_string_lower(sl_vm_t* vm, SLVAL self);

int
sl_string_byte_offset_for_index(sl_vm_t* vm, SLVAL str, int index);

int
sl_string_index_for_byte_offset(sl_vm_t* vm, SLVAL str, int byte_offset);

void
sl_init_string(sl_vm_t* vm);

void
sl_pre_init_string(sl_vm_t* vm);

#endif
