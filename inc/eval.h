#ifndef SL_EVAL_H
#define SL_EVAL_H

#include "vm.h"

typedef struct sl_eval_ctx {
    sl_vm_t* vm;
    SLVAL self;
    st_table_t* vars;
    struct sl_eval_ctx* parent;
}
sl_eval_ctx_t;

sl_eval_ctx_t*
sl_make_eval_ctx(sl_vm_t* vm);

sl_eval_ctx_t*
sl_close_eval_ctx(sl_vm_t* vm, sl_eval_ctx_t* ctx);

SLVAL
sl_do_file(sl_vm_t* vm, uint8_t* filename);

#include "ast.h"

SLVAL
sl_eval_seq(sl_node_seq_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_raw(sl_node_raw_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_echo(sl_node_echo_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_echo_raw(sl_node_echo_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_and(sl_node_binary_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_or(sl_node_binary_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_not(sl_node_unary_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_assign_seq(sl_node_assign_seq_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_assign_var(sl_node_assign_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_assign_ivar(sl_node_assign_ivar_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_assign_cvar(sl_node_assign_cvar_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_assign_const(sl_node_assign_const_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_immediate(sl_node_immediate_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_send(sl_node_send_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_const(sl_node_const_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_var(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_ivar(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_cvar(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_if(sl_node_if_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_for(sl_node_for_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_while(sl_node_while_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_class(sl_node_class_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_def(sl_node_def_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_try(sl_node_try_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_self(sl_node_base_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_array(sl_node_array_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_dict(sl_node_dict_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_return(sl_node_unary_t* node, sl_eval_ctx_t* ctx);

#endif
