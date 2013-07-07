#ifndef SL_METHOD_H
#define SL_METHOD_H

#include "value.h"
#include "vm.h"
#include "ast.h"
#include "eval.h"

extern struct sl_st_hash_type
sl_id_hash_type;

void
sl_init_method(sl_vm_t* vm);

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL klass, SLID name, int arity, SLVAL(*c_func)());

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLID name, sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx);

SLVAL
sl_method_bind(sl_vm_t* vm, SLVAL method, SLVAL receiver);

SLVAL
sl_id_to_string(sl_vm_t* vm, SLID id);

SLID
sl_intern2(sl_vm_t* vm, SLVAL str);

SLID
sl_intern(sl_vm_t* vm, char* cstr);

SLID
sl_id_make_setter(sl_vm_t* vm, SLID id);

#endif
