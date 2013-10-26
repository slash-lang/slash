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
    SLID op_div;
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
    SLID to_f;
    SLID subclassed;
};

struct sl_vm_lib {
    SLVAL _false;
    SLVAL _true;
    SLVAL ArgumentError;
    SLVAL Array;
    SLVAL Array_Enumerator;
    SLVAL Bignum;
    SLVAL BoundMethod;
    SLVAL Buffer;
    SLVAL Class;
    SLVAL Comparable;
    SLVAL CompileError;
    SLVAL Dict;
    SLVAL Dict_Enumerator;
    SLVAL EncodingError;
    SLVAL Enumerable;
    SLVAL Error;
    SLVAL Error_Frame;
    SLVAL False;
    SLVAL File;
    SLVAL File_InvalidOperation;
    SLVAL File_NotFound;
    SLVAL Float;
    SLVAL Int;
    SLVAL Lambda;
    SLVAL Method;
    SLVAL NameError;
    SLVAL Nil;
    SLVAL nil;
    SLVAL NoMethodError;
    SLVAL NotImplementedError;
    SLVAL Number;
    SLVAL Object;
    SLVAL Range_Enumerator;
    SLVAL Range_Exclusive;
    SLVAL Range_Inclusive;
    SLVAL Regexp;
    SLVAL Regexp_Match;
    SLVAL Request;
    SLVAL Response;
    SLVAL StackOverflowError;
    SLVAL StackOverflowError_instance;
    SLVAL String;
    SLVAL SyntaxError;
    SLVAL Time;
    SLVAL True;
    SLVAL TypeError;
    SLVAL ZeroDivisionError;
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
    SLVAL* store;
    int hash_seed;
    struct sl_mt_state* mt_state;
    struct sl_request_internal_opts* request;
    struct sl_response_internal_opts* response;
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

enum {
    SL_OP_RAW,
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
    SL_OP_SEND_SELF,
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
    SL_OP_RANGE_IN,
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
    SL_OP_BUILD_STRING,

    SL_OP__MAX_OPCODE
};

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

typedef size_t sl_vm_reg_t;
#define SL_MAX_REGISTERS (65536)

typedef union sl_vm_insn {
    sl_vm_opcode_t                  opcode;
    void*                           threaded_opcode;
    sl_vm_reg_t                     reg;
    size_t                          uint;
    struct sl_vm_section*           section;
    SLVAL                           imm;
    SLID                            id;
    sl_vm_inline_method_cache_t*    imc;
    sl_vm_inline_constant_cache_t*  icc;
}
sl_vm_insn_t;

typedef struct sl_vm_section {
    size_t insns_cap;
    size_t insns_count;
    sl_vm_insn_t* insns;

    int max_registers;
    int req_registers;
    int arg_registers;
    int has_extra_rest_arg;

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
            sl_method_t* method;
        } c_call_frame;
        struct {
            sl_vm_section_t* section;
            int* line;
        } sl_call_frame;
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

int
sl_vm_store_register_slot();

sl_vm_t*
sl_init(const char* sapi_name);

SLVAL
sl_vm_exec(sl_vm_exec_ctx_t* ctx, size_t ip);

#endif
