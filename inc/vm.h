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
    SLVAL SyntaxError;
    SLVAL EncodingError;
    SLVAL TypeError;
    SLVAL NoMethodError;
    SLVAL ZeroDivisionError;
    SLVAL NameError;
    
    SLVAL Nil;
    SLVAL nil;
    
    SLVAL True;
    SLVAL _true;
    
    SLVAL False;
    SLVAL _false;
    
    SLVAL Number;
    SLVAL Int;
    SLVAL Float;
    SLVAL Bignum;
    
    SLVAL Method;
    SLVAL BoundMethod;
}
sl_lib_t;

typedef struct sl_vm {
    int initializing;
    sl_lib_t lib;
    struct sl_catch_frame* catch_stack;
    void(*output)(struct sl_vm* ctx, char* buff, size_t len);
}
sl_vm_t;

#include "eval.h"

void
sl_static_init();

sl_vm_t*
sl_init();

#endif
