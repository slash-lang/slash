#ifndef SL_DISPATCH_H
#define SL_DISPATCH_H

#include <slash/value.h>

int
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id);

int
sl_responds_to2(sl_vm_t* vm, SLVAL object, SLID idv);

SLVAL
sl_apply_method(sl_vm_t* vm, SLVAL recv, sl_method_t* method, int argc, SLVAL* argv);

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, int argc, ...);

SLVAL
sl_send_id(sl_vm_t* vm, SLVAL recv, SLID id, int argc, ...);

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLID id, int argc, SLVAL* argv);

sl_method_t*
sl_lookup_method(sl_vm_t* vm, SLVAL recv, SLID id);

sl_method_t*
sl_lookup_method_in_class(sl_vm_t* vm, SLVAL klass, SLID id);

SLVAL
sl_imc_setup_call(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL receiver, SLVAL* argv);

SLVAL
sl_imc_setup_call2(sl_vm_t* vm, sl_vm_inline_method_cache_t* imc, SLVAL receiver, SLVAL klass, SLVAL* argv);

#endif
