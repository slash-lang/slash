#ifndef SL_DISPATCH_H
#define SL_DISPATCH_H

#include <slash/value.h>

int
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id);

int
sl_responds_to2(sl_vm_t* vm, SLVAL object, SLID idv);

SLVAL
sl_apply_method(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv);

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...);

SLVAL
sl_send_id(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, SLVAL* argv);

sl_method_t*
sl_lookup_method(sl_vm_t* vm, SLVAL recv, SLID id);

SLVAL
sl_imc_setup_call(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL receiver, SLVAL* argv);

#endif
