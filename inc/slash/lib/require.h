#ifndef SL_LIB_REQUIRE_H
#define SL_LIB_REQUIRE_H

#include <slash/vm.h>
#include <slash/value.h>

SLVAL
sl_require(sl_vm_t* vm, SLVAL self, SLVAL file);

void
sl_require_path_add(sl_vm_t* vm, char* path);

#endif
