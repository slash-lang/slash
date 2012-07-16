#include "parse.h"
#include "eval.h"
#include "string.h"
#include <gc.h>

sl_node_seq_t*
sl_make_seq_node()
{
    sl_node_seq_t* node = (sl_node_seq_t*)GC_MALLOC(sizeof(sl_node_seq_t));
    node->node_capacity = 2;
    node->node_count = 0;
    node->nodes = (sl_node_base_t**)GC_MALLOC(sizeof(sl_node_base_t*) * node->node_capacity);
    return node;
}

void
sl_seq_node_append(sl_node_seq_t* seq, sl_node_base_t* node)
{
    if(seq->node_count + 1 >= seq->node_capacity) {
        seq->node_capacity *= 2;
        seq->nodes = (sl_node_base_t**)GC_REALLOC(seq->nodes, sizeof(sl_node_base_t*) * seq->node_capacity);
    }
    seq->nodes[seq->node_count++] = node;
}

static sl_node_base_t*
sl_make_node(sl_node_type_t type, sl_node_eval_fn_t eval, size_t size)
{
    sl_node_base_t* node = (sl_node_base_t*)GC_MALLOC(size);
    node->type = type;
    node->eval = eval;
    return node;
}

#define MAKE_NODE(t, eval, type, block) do { \
        type* node = (type*)sl_make_node((t), (sl_node_eval_fn_t)&(eval), sizeof(type)); \
        block; \
        return (sl_node_base_t*)node; \
    } while(0)

sl_node_base_t*
sl_make_raw_node(sl_parse_state_t* ps, sl_token_t* token)
{
    MAKE_NODE(SL_NODE_RAW, sl_eval_raw, sl_node_raw_t, {
        node->string = sl_make_string(ps->vm, token->as.str.buff, token->as.str.len);
    });
}

sl_node_base_t*
sl_make_echo_node(sl_node_base_t* expr)
{
    MAKE_NODE(SL_NODE_ECHO, sl_eval_echo, sl_node_echo_t, {
        node->expr = expr;
    });
}

sl_node_base_t*
sl_make_raw_echo_node(sl_node_base_t* expr)
{
    MAKE_NODE(SL_NODE_ECHO_RAW, sl_eval_echo_raw, sl_node_echo_t, {
        node->expr = expr;
    });
}
