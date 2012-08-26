#ifndef SL_VM_H
#define SL_VM_H

#include "slash/value.h"
#include "slash/st.h"
#include "slash/error.h"
#include "slash/mem.h"

typedef struct sl_lib {
    SLVAL Object;
    SLVAL Class;
    SLVAL String;
    SLVAL Regexp;
    SLVAL Regexp_Match;
    
    SLVAL Error;
    SLVAL Error_Frame;
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
    SLVAL Range;
    SLVAL Range_Enumerator;
    
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
    void* stack_limit;
    char* cwd;
    st_table_t* globals;
    sl_gc_arena_t* arena;
}
sl_vm_t;

typedef enum sl_vm_opcode {
    SL_OP_RAW,
    SL_OP_ECHO,
    SL_OP_ECHO_RAW,
    SL_OP_NOT,
    SL_OP_MOV,
    SL_OP_GET_OUTER,
    SL_OP_GET_GLOBAL,
    SL_OP_GET_IVAR,
    SL_OP_GET_CVAR,
    SL_OP_GET_CONST,
    SL_OP_GET_OBJECT_CONST,
    SL_OP_SET_OUTER,
    SL_OP_SET_GLOBAL,
    SL_OP_SET_IVAR,
    SL_OP_SET_CVAR,
    SL_OP_SET_CONST,
    SL_OP_SET_OBJECT_CONST,
    SL_OP_IMMEDIATE,
    SL_OP_SEND,
    SL_OP_JUMP,
    SL_OP_JUMP_IF,
    SL_OP_JUMP_UNLESS,
    SL_OP_CLASS,
    SL_OP_DEFINE,
    SL_OP_DEFINE_ON,
    SL_OP_LAMBDA,
    SL_OP_SELF,
    SL_OP_ARRAY,
    SL_OP_ARRAY_DUMP,
    SL_OP_DICT,
    SL_OP_RETURN,
    SL_OP_RANGE,
    SL_OP_RANGE_EX,
    SL_OP_LINE_TRACE
}
sl_vm_opcode_t;

typedef union sl_vm_insn {
    sl_vm_opcode_t        opcode;
    size_t                uint;
    struct sl_vm_section* section;
    SLVAL                 imm;
    sl_string_t*          str;
}
sl_vm_insn_t;

typedef struct sl_vm_section {
    size_t insns_count;
    size_t insns_cap;
    sl_vm_insn_t* insns;
    size_t max_registers;
    size_t arg_registers;
    int can_stack_alloc_frame;
}
sl_vm_section_t;

typedef struct sl_vm_exec_ctx {
    sl_vm_t* vm;
    sl_vm_section_t* section;
    SLVAL* registers;
    SLVAL self;
    struct sl_vm_exec_ctx* parent;
}
sl_vm_exec_ctx_t;

#include "slash/eval.h"

void
sl_static_init();

sl_vm_t*
sl_init();

SLVAL
sl_vm_store_get(sl_vm_t* vm, void* key);

void
sl_vm_store_put(sl_vm_t* vm, void* key, SLVAL val);

SLVAL
sl_get_global(sl_vm_t* vm, char* name);

SLVAL
sl_get_global2(sl_vm_t* vm, sl_string_t* name);

void
sl_set_global(sl_vm_t* vm, char* name, SLVAL val);

void
sl_set_global2(sl_vm_t* vm, sl_string_t* name, SLVAL val);

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx);

#endif
