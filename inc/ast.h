#ifndef SL_AST_H
#define SL_AST_H

#include "value.h"

typedef enum sl_node_type {
    SL_NODE_SEQ,
    SL_NODE_RAW,
    SL_NODE_ECHO,
    SL_NODE_ECHO_RAW,
    SL_NODE_VAR,
    SL_NODE_IVAR,
    SL_NODE_CVAR,
    SL_NODE_IMMEDIATE,
    SL_NODE_SELF,
    SL_NODE_CLASS,
    SL_NODE_DEF,
    SL_NODE_LAMBDA,
    SL_NODE_TRY,
    SL_NODE_IF,
    SL_NODE_UNLESS,
    SL_NODE_FOR,
    SL_NODE_WHILE,
    SL_NODE_UNTIL,
    SL_NODE_SEND,
    SL_NODE_CONST,
    SL_NODE_AND,
    SL_NODE_OR,
    SL_NODE_NOT,
    SL_NODE_ASSIGN_VAR,
    SL_NODE_ASSIGN_IVAR,
    SL_NODE_ASSIGN_CVAR,
    SL_NODE_ASSIGN_SEND,
    SL_NODE_ASSIGN_CONST,
    SL_NODE_ASSIGN_ARRAY,
    SL_NODE_ARRAY,
    SL_NODE_DICT,
    SL_NODE_RETURN
}
sl_node_type_t;

struct sl_eval_ctx;
struct sl_node_base;

typedef struct sl_node_base {
    sl_node_type_t type;
    SLVAL(*eval)(struct sl_node_base*, struct sl_eval_ctx*);
}
sl_node_base_t;

typedef SLVAL(*sl_node_eval_fn_t)(sl_node_base_t*, struct sl_eval_ctx*);

typedef struct sl_node_seq {
    sl_node_base_t base;
    sl_node_base_t** nodes;
    size_t node_count;
    size_t node_capacity;
}
sl_node_seq_t;

typedef struct sl_node_raw {
    sl_node_base_t base;
    SLVAL string;
}
sl_node_raw_t;

typedef struct sl_node_echo {
    sl_node_base_t base;
    sl_node_base_t* expr;
}
sl_node_echo_t;

typedef struct sl_node_var {
    sl_node_base_t base;
    sl_string_t* name;
}
sl_node_var_t;

typedef struct sl_node_immediate {
    sl_node_base_t base;
    SLVAL value;
}
sl_node_immediate_t;

typedef struct sl_node_class {
    sl_node_base_t base;
    SLVAL name;
    sl_node_base_t* extends;
    sl_node_base_t* body;
}
sl_node_class_t;

typedef struct sl_node_def {
    sl_node_base_t base;
    sl_node_base_t* on;
    SLVAL name;
    sl_string_t** args;
    size_t arg_count;
    sl_node_base_t* body;
}
sl_node_def_t;

typedef struct sl_node_lambda {
    sl_node_base_t base;
    sl_string_t** args;
    size_t arg_count;
    sl_node_base_t* body;
}
sl_node_lambda_t;

typedef struct sl_node_if {
    sl_node_base_t base;
    sl_node_base_t* condition;
    sl_node_base_t* body;
    sl_node_base_t* else_body;
}
sl_node_if_t;

typedef struct sl_node_for {
    sl_node_base_t base;
    sl_node_base_t* lval;
    sl_node_base_t* expr;
    sl_node_base_t* body;
    sl_node_base_t* else_body;
}
sl_node_for_t;

typedef struct sl_node_while {
    sl_node_base_t base;
    sl_node_base_t* expr;
    sl_node_base_t* body;
}
sl_node_while_t;

typedef struct sl_node_try {
    sl_node_base_t base;
    sl_node_base_t* body;
    sl_node_base_t* lval;
    sl_node_base_t* catch_body;
}
sl_node_try_t;

typedef struct sl_node_send {
    sl_node_base_t base;
    uint8_t* file;
    int line;
    sl_node_base_t* recv;
    SLVAL id;
    sl_node_base_t** args;
    size_t arg_count;
}
sl_node_send_t;

typedef struct sl_node_const_get {
    sl_node_base_t base;
    sl_node_base_t* obj;
    SLVAL id;
}
sl_node_const_t;

typedef struct sl_node_binary {
    sl_node_base_t base;
    sl_node_base_t* left;
    sl_node_base_t* right;
}
sl_node_binary_t;

typedef struct sl_node_unary {
    sl_node_base_t base;
    sl_node_base_t* expr;
}
sl_node_unary_t;

typedef struct sl_node_array {
    sl_node_base_t base;
    size_t node_count;
    sl_node_base_t** nodes;
}
sl_node_array_t;

typedef struct sl_node_dict {
    sl_node_base_t base;
    size_t node_count;
    sl_node_base_t** keys;
    sl_node_base_t** vals;
}
sl_node_dict_t;

typedef struct sl_node_assign_var {
    sl_node_base_t base;
    sl_node_var_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_var_t;

typedef struct sl_node_assign_ivar {
    sl_node_base_t base;
    sl_node_var_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_ivar_t;

typedef struct sl_node_assign_cvar {
    sl_node_base_t base;
    sl_node_var_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_cvar_t;

typedef struct sl_node_assign_send {
    sl_node_base_t base;
    sl_node_send_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_send_t;

typedef struct sl_node_assign_const {
    sl_node_base_t base;
    sl_node_const_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_const_t;

typedef struct sl_node_assign_array {
    sl_node_base_t base;
    sl_node_array_t* lval;
    sl_node_base_t* rval;
}
sl_node_assign_array_t;

#endif
