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
    SL_NODE_INTERP_STRING,
    SL_NODE_SELF,
    SL_NODE_CLASS,
    SL_NODE_DEF,
    SL_NODE_LAMBDA,
    SL_NODE_TRY,
    SL_NODE_IF,
    SL_NODE_SWITCH,
    SL_NODE_FOR,
    SL_NODE_WHILE,
    SL_NODE_SEND,
    SL_NODE_SUPER,
    SL_NODE_BIND_METHOD,
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
    SL_NODE_PREFIX_MUTATE,
    SL_NODE_POSTFIX_MUTATE,
    SL_NODE_ARRAY,
    SL_NODE_DICT,
    SL_NODE_RETURN,
    SL_NODE_RANGE,
    SL_NODE_NEXT,
    SL_NODE_LAST,
    SL_NODE_THROW,
    SL_NODE_YADA_YADA,
    SL_NODE_USE,
    SL_NODE__REGISTER
}
sl_node_type_t;

typedef struct {
    sl_node_type_t type;
    int line;
}
sl_node_base_t;

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

typedef struct sl_node_interp_string {
    sl_node_base_t base;
    size_t components_count;
    sl_node_base_t** components;
}
sl_node_interp_string_t;

typedef struct sl_node_class {
    sl_node_base_t base;
    SLID name;
    SLVAL doc;
    sl_node_base_t* extends;
    sl_node_base_t* body;
}
sl_node_class_t;

typedef struct sl_node_opt_arg {
    sl_string_t* name;
    sl_node_base_t* default_value;
}
sl_node_opt_arg_t;

typedef struct sl_node_def {
    sl_node_base_t base;
    sl_node_base_t* on;
    SLID name;
    SLVAL doc;
    sl_string_t** req_args;
    size_t req_arg_count;
    sl_node_opt_arg_t* opt_args;
    size_t opt_arg_count;
    sl_node_base_t* body;
    sl_string_t* rest_arg;
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

typedef struct sl_node_switch_case {
    sl_node_base_t* value;
    sl_node_base_t* body;
}
sl_node_switch_case_t;

typedef struct sl_node_switch {
    sl_node_base_t base;
    sl_node_base_t* value;
    size_t case_count;
    sl_node_switch_case_t* cases;
    sl_node_base_t* else_body;
}
sl_node_switch_t;

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
    sl_node_base_t* recv;
    SLID id;
    sl_node_base_t** args;
    size_t arg_count;
}
sl_node_send_t;

typedef struct sl_node_super {
    sl_node_base_t base;
    sl_node_base_t** args;
    size_t arg_count;
}
sl_node_super_t;

typedef struct sl_node_bind_method {
    sl_node_base_t base;
    sl_node_base_t* recv;
    SLID id;
}
sl_node_bind_method_t;

typedef struct sl_node_const_get {
    sl_node_base_t base;
    sl_node_base_t* obj;
    SLID id;
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
    char* op_method;
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

typedef struct sl_node_mutate {
    sl_node_base_t base;
    sl_node_base_t* lval;
    char* op_method;
}
sl_node_mutate_t;

typedef struct sl_node_range {
    sl_node_base_t base;
    sl_node_base_t* left;
    sl_node_base_t* right;
    int exclusive;
}
sl_node_range_t;

typedef struct sl_node__register {
    sl_node_base_t base;
    size_t reg;
}
sl_node__register_t;

#endif
