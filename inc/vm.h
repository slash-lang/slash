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
    SLVAL ArgumentError;
    SLVAL EncodingError;
    SLVAL NameError;
    SLVAL NoMethodError;
    SLVAL StackOverflowError;
    SLVAL SyntaxError;
    SLVAL TypeError;
    SLVAL ZeroDivisionError;
    
    SLVAL Nil;
    SLVAL nil;
    
    SLVAL True;
    SLVAL _true;
    
    SLVAL False;
    SLVAL _false;
    
    SLVAL Comparable;
    SLVAL Number;
    SLVAL Int;
    SLVAL Float;
    SLVAL Bignum;
    
    SLVAL Enumerable;
    SLVAL Array;
    SLVAL Array_Enumerator;
    SLVAL Dict;
    SLVAL Dict_Enumerator;
    
    SLVAL File;
    
    SLVAL Method;
    SLVAL BoundMethod;
    SLVAL Lambda;
}
sl_lib_t;

typedef struct sl_vm {
    int initializing;
    sl_lib_t lib;
    struct sl_catch_frame* catch_stack;
    void* data;
    st_table_t* store;
    int hash_seed;
}
sl_vm_t;

#include "eval.h"

void
sl_static_init();

sl_vm_t*
sl_init();

SLVAL
sl_vm_store_get(sl_vm_t* vm, void* key);

void
sl_vm_store_put(sl_vm_t* vm, void* key, SLVAL val);

#endif
