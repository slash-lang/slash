#ifndef SL_METHOD_H
#define SL_METHOD_H

#include "value.h"
#include "vm.h"
#include "ast.h"
#include "eval.h"

void
sl_init_method(sl_vm_t* vm);

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*c_func)());

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, size_t arg_count, sl_string_t** args, sl_node_base_t* body, sl_eval_ctx_t* ctx);

#endif
