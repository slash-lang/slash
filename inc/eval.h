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

sl_eval_ctx_t* sl_make_eval_ctx(sl_vm_t* vm);

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
sl_eval_assign(sl_node_binary_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_immediate(sl_node_immediate_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_send(sl_node_send_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_constant(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_var(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_ivar(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_cvar(sl_node_var_t* node, sl_eval_ctx_t* ctx);

#endif
