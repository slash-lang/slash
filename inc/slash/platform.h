#ifndef SL_PLATFORM_H
#define SL_PLATFORM_H

#include "vm.h"

char* sl_realpath(sl_vm_t* vm, char* path);
int sl_file_exists(sl_vm_t* vm, char* path);
int sl_abs_file_exists(char* path);
int sl_seed();
void* sl_stack_limit();

#ifdef WIN32
    #include <malloc.h>
#else
    #include <alloca.h>
#endif

#endif
