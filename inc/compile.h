#ifndef SL_COMPILE_H
#define SL_COMPILE_H

#include "vm.h"
#include "ast.h"

sl_vm_section_t*
sl_compile(sl_vm_t* vm, sl_node_base_t* ast);

#endif
