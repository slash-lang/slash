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
    SLID subclassed;
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

    SLVAL Buffer;
    
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

typedef enum {
    SL_INIT_RAND
}
sl_vm_init_flag_t;

typedef struct sl_vm {
    int initializing;
    struct sl_vm_lib lib;
    struct sl_vm_ids id;
    struct sl_vm_frame* call_stack;
    void* data;
    sl_st_table_t* store;
    int hash_seed;
    void* stack_limit;
    char* cwd;
    sl_gc_arena_t* arena;
    sl_st_table_t* required;
    uint32_t state_constant;
    uint32_t state_method;
    sl_vm_init_flag_t inited;
    struct {
        sl_st_table_t* name_to_id;
        SLVAL* id_to_name;
        size_t id_to_name_cap;
        size_t id_to_name_size;
    } intern;
}
sl_vm_t;

#define SL_OP_RAW                   1
#define SL_OP_ECHO                  2
#define SL_OP_ECHO_RAW              3
#define SL_OP_NOT                   4
#define SL_OP_MOV                   5
#define SL_OP_GET_OUTER             6
#define SL_OP_GET_IVAR              7
#define SL_OP_GET_CVAR              8
#define SL_OP_GET_CONST             9
#define SL_OP_GET_OBJECT_CONST      10
#define SL_OP_SET_OUTER             11
#define SL_OP_SET_IVAR              12
#define SL_OP_SET_CVAR              13
#define SL_OP_SET_CONST             14
#define SL_OP_SET_OBJECT_CONST      15
#define SL_OP_IMMEDIATE             16
#define SL_OP_SEND                  17
#define SL_OP_JUMP                  18
#define SL_OP_JUMP_IF               19
#define SL_OP_JUMP_UNLESS           20
#define SL_OP_CLASS                 21
#define SL_OP_DEFINE                22
#define SL_OP_DEFINE_ON             23
#define SL_OP_LAMBDA                24
#define SL_OP_SELF                  25
#define SL_OP_ARRAY                 26
#define SL_OP_ARRAY_DUMP            27
#define SL_OP_DICT                  28
#define SL_OP_RETURN                29
#define SL_OP_RANGE                 30
#define SL_OP_RANGE_EX              31
#define SL_OP_LINE_TRACE            32
#define SL_OP_ABORT                 33
#define SL_OP_THROW                 34
#define SL_OP_TRY                   35
#define SL_OP_END_TRY               36
#define SL_OP_CATCH                 37
#define SL_OP_YADA_YADA             38
#define SL_OP_BIND_METHOD           39
#define SL_OP_USE                   40
#define SL_OP_USE_TOP_LEVEL         41
#define SL_OP_BUILD_STRING          42

typedef uint8_t sl_vm_opcode_t;

typedef struct sl_vm_inline_method_cache {
    uint32_t state;
    SLVAL klass;
    sl_method_t* method;
    SLID id;
    int argc;
    SLVAL(*call)(struct sl_vm*,struct sl_vm_inline_method_cache*,SLVAL,SLVAL*);
}
sl_vm_inline_method_cache_t;

typedef struct sl_vm_inline_constant_cache {
    uint32_t state;
    SLVAL klass;
    SLVAL value;
}
sl_vm_inline_constant_cache_t;

typedef union sl_vm_insn {
    sl_vm_opcode_t                  opcode;
    size_t                          reg;
    size_t                          uint;
    struct sl_vm_section*           section;
    SLVAL                           imm;
    SLID                            id;
    sl_vm_inline_method_cache_t*    imc;
    sl_vm_inline_constant_cache_t*  icc;
}
sl_vm_insn_t;

typedef struct sl_vm_section {
    size_t insns_byte_count;
    size_t insns_byte_cap;
    char* insns_bytes;

    int max_registers;
    int req_registers;
    int arg_registers;
    int has_extra_rest_arg;
    
    size_t* opt_skip;
    uint8_t* filename;
    bool can_stack_alloc_frame;
    bool has_try_catch;
    SLID name;

    size_t gc_list_count;
    size_t gc_list_cap;
    sl_vm_insn_t* gc_list;
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
