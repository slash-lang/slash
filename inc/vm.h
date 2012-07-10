#ifndef SL_VM_H
#define SL_VM_H

#include "value.h"
#include "st.h"
#include "error.h"

typedef struct sl_lib {
    SLVAL Object;
    SLVAL Class;
    SLVAL String;
    SLVAL Error;
    
    SLVAL Nil;
    SLVAL nil;
    
    SLVAL Number;
    SLVAL Int;
    SLVAL Float;
    SLVAL Bignum;
    
    SLVAL Method;
    SLVAL BoundMethod;
}
sl_lib_t;

typedef struct sl_vm {
    st_table_t* globals;
    int initializing;
    sl_lib_t lib;
    struct sl_catch_frame* catch_stack;
}
sl_vm_t;

void
sl_static_init();

sl_vm_t*
sl_init();

#endif