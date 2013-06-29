#ifndef SL_LIB_REGEXP_H
#define SL_LIB_REGEXP_H

#include <slash/vm.h>
#include <slash/value.h>

SLVAL
sl_make_regexp(sl_vm_t* vm, uint8_t* re, size_t re_len, uint8_t* opts, size_t opts_len);

SLVAL
sl_regexp_match(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

SLVAL
sl_regexp_is_match(sl_vm_t* vm, SLVAL self, SLVAL other);

SLVAL
sl_regexp_match_byte_offset(sl_vm_t* vm, SLVAL self, SLVAL i);

SLVAL
sl_regexp_match_index(sl_vm_t* vm, SLVAL self, SLVAL i);

SLVAL
sl_regexp_match_capture(sl_vm_t* vm, SLVAL self, SLVAL i);

SLVAL
sl_regexp_match_before(sl_vm_t* vm, SLVAL self);

SLVAL
sl_regexp_match_after(sl_vm_t* vm, SLVAL self);

#endif
