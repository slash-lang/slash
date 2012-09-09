#ifndef SL_LIB_LAMBDA_H
#define SL_LIB_LAMBDA_H

#include <slash/vm.h>
#include <slash/value.h>

SLVAL
sl_make_lambda(sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx);

#endif
