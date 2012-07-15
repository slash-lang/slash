#ifndef SL_EVAL_H
#define SL_EVAL_H

#include "vm.h"

typedef struct sl_eval_ctx {
    sl_vm_t* vm;
    SLVAL self;
}
sl_eval_ctx_t;

#include "ast.h"

SLVAL
sl_eval_seq(sl_node_seq_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_raw(sl_node_raw_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_echo(sl_node_echo_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_echo_raw(sl_node_echo_t* node, sl_eval_ctx_t* ctx);

#endif
