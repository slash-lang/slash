#ifndef SL_PARSE_H
#define SL_PARSE_H

#include "vm.h"
#include "lex.h"
#include "ast.h"
#include "eval.h"
#include "st.h"

typedef enum {
    SL_PF_CAN_RETURN    = 1,
    SL_PF_CAN_NEXT_LAST = 2,
    SL_PF_SCOPE_CLOSURE = 4
}
sl_parse_flags_t;

typedef struct sl_parse_scope {
    struct sl_parse_scope* prev;
    sl_parse_flags_t flags;
}
sl_parse_scope_t;

typedef struct {
    sl_vm_t* vm;
    sl_token_t* tokens;
    size_t token_count;
    size_t current_token;
    uint8_t* filename;
    sl_node_base_t* ast;
    sl_parse_scope_t* scope;
    int line;
}
sl_parse_state_t;

sl_node_base_t*
sl_parse(sl_vm_t* vm, sl_token_t* tokens, size_t token_count, uint8_t* filename);

void
sl_parse_error(sl_parse_state_t* ps, char* message);

int
sl_node_is_lval(sl_node_base_t* node);

sl_node_base_t*
sl_make_singleton_node(sl_parse_state_t* ps, sl_node_type_t type);

sl_node_seq_t*
sl_make_seq_node(sl_parse_state_t* ps);

void
sl_seq_node_append(sl_parse_state_t* ps, sl_node_seq_t* seq, sl_node_base_t* node);

sl_node_base_t*
sl_make_raw_node(sl_parse_state_t* ps, sl_token_t* token);

sl_node_base_t*
sl_make_echo_node(sl_parse_state_t* ps, sl_node_base_t* expr);

sl_node_base_t*
sl_make_raw_echo_node(sl_parse_state_t* ps, sl_node_base_t* expr);

sl_node_base_t*
sl_make_binary_node(sl_parse_state_t* ps, sl_node_base_t* left, sl_node_base_t* right, sl_node_type_t type);

sl_node_base_t*
sl_make_unary_node(sl_parse_state_t* ps, sl_node_base_t* expr, sl_node_type_t type);

sl_node_base_t*
sl_make_immediate_node(sl_parse_state_t* ps, SLVAL val);

sl_node_base_t*
sl_make_send_node(sl_parse_state_t* ps, sl_node_base_t* recv, SLVAL id, size_t argc, sl_node_base_t** argv);

sl_node_base_t*
sl_make_var_node(sl_parse_state_t* ps, sl_node_type_t type, SLVAL id);

sl_node_base_t*
sl_make_const_node(sl_parse_state_t* ps, sl_node_base_t* obj, SLVAL id);

sl_node_base_t*
sl_make_if_node(sl_parse_state_t* ps, sl_node_base_t* cond, sl_node_base_t* if_true, sl_node_base_t* if_false);

sl_node_base_t*
sl_make_while_node(sl_parse_state_t* ps, sl_node_base_t* expr, sl_node_base_t* body);

sl_node_base_t*
sl_make_for_node(sl_parse_state_t* ps, sl_node_base_t* lval, sl_node_base_t* expr, sl_node_base_t* body, sl_node_base_t* else_body);

sl_node_base_t*
sl_make_class_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* extends, sl_node_base_t* body);

sl_node_base_t*
sl_make_def_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* on, size_t arg_count, sl_string_t** args, sl_node_base_t* body);

sl_node_base_t*
sl_make_lambda_node(sl_parse_state_t* ps, size_t arg_count, sl_string_t** args, sl_node_base_t* body);

sl_node_base_t*
sl_make_try_node(sl_parse_state_t* ps, sl_node_base_t* body, sl_node_base_t* lval, sl_node_base_t* catch_body);

sl_node_base_t*
sl_make_self_node(sl_parse_state_t* ps);

sl_node_base_t*
sl_make_assign_var_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_assign_ivar_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_assign_cvar_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_assign_global_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_assign_send_node(sl_parse_state_t* ps, sl_node_send_t* lval, sl_node_base_t* rval, char* op_method);

sl_node_base_t*
sl_make_assign_const_node(sl_parse_state_t* ps, sl_node_const_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_assign_array_node(sl_parse_state_t* ps, sl_node_array_t* lval, sl_node_base_t* rval);

sl_node_base_t*
sl_make_simple_assign_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval, char* op_method);

sl_node_base_t*
sl_make_prefix_mutate_node(sl_parse_state_t* ps, sl_node_base_t* lval, char* op_method);

sl_node_base_t*
sl_make_postfix_mutate_node(sl_parse_state_t* ps, sl_node_base_t* lval, char* op_method);

sl_node_base_t*
sl_make_array_node(sl_parse_state_t* ps, size_t node_count, sl_node_base_t** nodes);

sl_node_base_t*
sl_make_dict_node(sl_parse_state_t* ps, size_t node_count, sl_node_base_t** keys, sl_node_base_t** vals);

sl_node_base_t*
sl_make_range_node(sl_parse_state_t* ps, sl_node_base_t* left, sl_node_base_t* right, int exclusive);

#endif
