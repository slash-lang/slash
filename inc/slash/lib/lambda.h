#ifndef SL_LIB_LAMBDA_H
#define SL_LIB_LAMBDA_H

#include <slash/vm.h>
#include <slash/value.h>

SLVAL
sl_make_lambda(sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx);

SLVAL
sl_lambda_call_with_self(sl_vm_t* vm, SLVAL lambda, SLVAL self, size_t argc, SLVAL* argv);

SLVAL
sl_lambda_call(sl_vm_t* vm, SLVAL lambda, size_t argc, SLVAL* argv);

SLVAL
sl_lambda_to_method(sl_vm_t* vm, SLVAL lambda);

#endif
