#include "parse.h"
#include "eval.h"
#include "string.h"
#include "class.h"
#include "lib/lambda.h"
#include <gc.h>
#include <string.h>

int
sl_node_is_lval(sl_node_base_t* node)
{
    switch(node->type) {
        case SL_NODE_VAR:   return 1;
        case SL_NODE_IVAR:  return 1;
        case SL_NODE_CVAR:  return 1;
        case SL_NODE_CONST: return 1;
        case SL_NODE_SEND:  return 1;
        default:
            return 0;
    }
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
        type* node = (type*)sl_make_node((t), (sl_node_eval_fn_t)(eval), sizeof(type)); \
        block; \
        return (sl_node_base_t*)node; \
    } while(0)

sl_node_seq_t*
sl_make_seq_node()
{
    sl_node_seq_t* node = (sl_node_seq_t*)
        sl_make_node(SL_NODE_SEQ, (sl_node_eval_fn_t)&sl_eval_seq, sizeof(sl_node_seq_t));
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

sl_node_base_t*
sl_make_binary_node(sl_node_base_t* left, sl_node_base_t* right, sl_node_type_t type, SLVAL(*eval)(sl_node_binary_t*,sl_eval_ctx_t*))
{
    MAKE_NODE(type, eval, sl_node_binary_t, {
        node->left = left;
        node->right = right;
    });
}

sl_node_base_t*
sl_make_unary_node(sl_node_base_t* expr, sl_node_type_t type, SLVAL(*eval)(sl_node_unary_t*,sl_eval_ctx_t*))
{
    MAKE_NODE(type, eval, sl_node_unary_t, {
        node->expr = expr;
    });
}

sl_node_base_t*
sl_make_immediate_node(SLVAL val)
{
    MAKE_NODE(SL_NODE_IMMEDIATE, sl_eval_immediate, sl_node_immediate_t, {
        node->value = val;
    });
}

sl_node_base_t*
sl_make_send_node(sl_parse_state_t* ps, int line, sl_node_base_t* recv, SLVAL id, size_t argc, sl_node_base_t** argv)
{
    MAKE_NODE(SL_NODE_SEND, sl_eval_send, sl_node_send_t, {
        node->file = ps->filename;
        node->line = line;
        node->recv = recv;
        node->id = id;
        node->arg_count = argc;
        node->args = GC_MALLOC(sizeof(sl_node_base_t*) * argc);
        memcpy(node->args, argv, sizeof(sl_node_base_t*) * argc);
    });
}

sl_node_base_t*
sl_make_var_node(sl_parse_state_t* ps, sl_node_type_t type, SLVAL(*eval)(sl_node_var_t*,sl_eval_ctx_t*), SLVAL id)
{
    MAKE_NODE(type, eval, sl_node_var_t, {
        node->name = (sl_string_t*)sl_get_ptr(sl_expect(ps->vm, id, ps->vm->lib.String));
    });
}

sl_node_base_t*
sl_make_const_node(sl_node_base_t* obj, SLVAL id)
{
    MAKE_NODE(SL_NODE_CONST, sl_eval_const, sl_node_const_t, {
        node->obj = obj;
        node->id = id;
    });
}

sl_node_base_t*
sl_make_if_node(sl_node_base_t* cond, sl_node_base_t* if_true, sl_node_base_t* if_false)
{
    MAKE_NODE(SL_NODE_IF, sl_eval_if, sl_node_if_t, {
        node->condition = cond;
        node->body = if_true;
        node->else_body = if_false;
    });
}

sl_node_base_t*
sl_make_while_node(sl_node_base_t* expr, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_WHILE, sl_eval_while, sl_node_while_t, {
        node->expr = expr;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_for_node(sl_node_base_t* lval, sl_node_base_t* expr, sl_node_base_t* body, sl_node_base_t* else_body)
{
    MAKE_NODE(SL_NODE_FOR, sl_eval_for, sl_node_for_t, {
        node->lval = lval;
        node->expr = expr;
        node->body = body;
        node->else_body = else_body;
    });
}

sl_node_base_t*
sl_make_class_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* extends, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_CLASS, sl_eval_class, sl_node_class_t, {
        sl_expect(ps->vm, name, ps->vm->lib.String);
        node->name = name;
        node->extends = extends;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_def_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* on, size_t arg_count, sl_string_t** args, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_DEF, sl_eval_def, sl_node_def_t, {
        sl_expect(ps->vm, name, ps->vm->lib.String);
        node->name = name;
        node->on = on;
        node->args = args;
        node->arg_count = arg_count;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_lambda_node(size_t arg_count, sl_string_t** args, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_LAMBDA, sl_make_lambda, sl_node_lambda_t, {
        node->args = args;
        node->arg_count = arg_count;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_self_node()
{
    MAKE_NODE(SL_NODE_SELF, sl_eval_self, sl_node_base_t, {});
}

sl_node_base_t*
sl_make_assign_var_node(sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_VAR, sl_eval_assign_var, sl_node_assign_var_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_ivar_node(sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_IVAR, sl_eval_assign_ivar, sl_node_assign_ivar_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_cvar_node(sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_CVAR, sl_eval_assign_cvar, sl_node_assign_cvar_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_const_node(sl_node_const_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_CONST, sl_eval_assign_const, sl_node_assign_const_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_array_node(size_t node_count, sl_node_base_t** nodes)
{
    MAKE_NODE(SL_NODE_ARRAY, sl_eval_array, sl_node_array_t, {
        node->node_count = node_count;
        node->nodes = nodes;
    });
}

sl_node_base_t*
sl_make_dict_node(size_t node_count, sl_node_base_t** keys, sl_node_base_t** vals)
{
    MAKE_NODE(SL_NODE_DICT, sl_eval_dict, sl_node_dict_t, {
        node->node_count = node_count;
        node->keys = keys;
        node->vals = vals;
    });
}
