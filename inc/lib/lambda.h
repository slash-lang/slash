#ifndef SL_LIB_LAMBDA_H
#define SL_LIB_LAMBDA_H

#include "vm.h"
#include "value.h"

SLVAL
sl_make_lambda(sl_node_lambda_t* node, sl_eval_ctx_t* ctx);

#endif