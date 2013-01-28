#ifndef SL_VM_H
#define SL_VM_H

#include <stdbool.h>
#include "value.h"
#include "st.h"
#include "error.h"
#include "mem.h"

struct sl_vm_ids {
    /* operators */
    SLID op_cmp;
    SLID op_eq;
    SLID op_lt;
    SLID op_lte;
    SLID op_add;
    SLID op_sub;
    /* methods */
    SLID call;
    SLID current;
    SLID enumerate;
    SLID hash;
    SLID init;
    SLID inspect;
    SLID method_missing;
    SLID next;
    SLID succ;
    SLID to_s;
};

struct sl_vm_lib {
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
    SLVAL StackOverflowError_instance;
    SLVAL SyntaxError;
    SLVAL TypeError;
    SLVAL ZeroDivisionError;
    SLVAL NotImplementedError;
    
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
    
    SLVAL Time;
};

typedef struct sl_vm {
    int initializing;
    struct sl_vm_lib lib;
    struct sl_vm_ids id;
    struct sl_vm_frame* call_stack;
    void* data;
    st_table_t* store;
    int hash_seed;
    void* stack_limit;
    char* cwd;
    sl_gc_arena_t* arena;
    st_table_t* required;
    uint32_t state_constant;
    uint32_t state_method;
    struct {
        st_table_t* name_to_id;
        SLVAL* id_to_name;
        size_t id_to_name_cap;
        size_t id_to_name_size;
    } intern;
}
sl_vm_t;

typedef enum sl_vm_opcode {
    SL_OP_RAW = 1,
    SL_OP_ECHO,
    SL_OP_ECHO_RAW,
    SL_OP_NOT,
    SL_OP_MOV,
    SL_OP_GET_OUTER,
    SL_OP_GET_IVAR,
    SL_OP_GET_CVAR,
    SL_OP_GET_CONST,
    SL_OP_GET_OBJECT_CONST,
    SL_OP_SET_OUTER,
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
    SL_OP_LINE_TRACE,
    SL_OP_ABORT,
    SL_OP_THROW,
    SL_OP_TRY,
    SL_OP_END_TRY,
    SL_OP_CATCH,
    SL_OP_YADA_YADA,
    SL_OP_BIND_METHOD,
    SL_OP_USE,
    SL_OP_USE_TOP_LEVEL,
    SL_OP_BUILD_STRING
}
sl_vm_opcode_t;

typedef struct sl_vm_inline_cache {
    uint32_t state;
    SLVAL klass;
    SLVAL value;
}
sl_vm_inline_cache_t;

typedef union sl_vm_insn {
    sl_vm_opcode_t          opcode;
    size_t                  uint;
    struct sl_vm_section*   section;
    SLVAL                   imm;
    SLID                    id;
    sl_string_t*            str;
    sl_vm_inline_cache_t*   ic;
}
sl_vm_insn_t;

typedef struct sl_vm_section {
    size_t insns_count;
    size_t insns_cap;
    sl_vm_insn_t* insns;
    size_t max_registers;
    size_t req_registers;
    size_t arg_registers;
    size_t* opt_skip;
    uint8_t* filename;
    bool can_stack_alloc_frame;
    bool has_try_catch;
    SLID name;
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

typedef enum sl_vm_frame_type {
    SL_VM_FRAME_SLASH,
    SL_VM_FRAME_C,
    SL_VM_FRAME_HANDLER
}
sl_vm_frame_type_t;

typedef struct sl_vm_frame {
    struct sl_vm_frame* prev;
    sl_vm_frame_type_t frame_type;
    
    union {
        struct {
            SLID method;
            /* left undefined if type == SL_VM_FRAME_C: */
            char* filename;
            int* line;
        } call_frame;
        struct {
            jmp_buf env;
            SLVAL value;
            sl_unwind_type_t unwind_type;
        } handler_frame;
    } as;
}
sl_vm_frame_t;

#include "eval.h"

void
sl_static_init();

sl_vm_t*
sl_init(const char* sapi_name);

SLVAL
sl_vm_store_get(sl_vm_t* vm, void* key);

void
sl_vm_store_put(sl_vm_t* vm, void* key, SLVAL val);

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx, size_t ip);

#endif
