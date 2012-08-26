#ifndef SL_METHOD_H
#define SL_METHOD_H

#include "slash/value.h"
#include "slash/vm.h"
#include "slash/ast.h"
#include "slash/eval.h"

void
sl_init_method(sl_vm_t* vm);

SLVAL
sl_make_c_func(sl_vm_t* vm, SLVAL klass, SLVAL name, int arity, SLVAL(*c_func)());

SLVAL
sl_make_method(sl_vm_t* vm, SLVAL klass, SLVAL name, sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx);

#endif
