#include "parse.h"
#include "eval.h"
#include "string.h"
#include "class.h"
#include "lib/lambda.h"
#include "object.h"
#include <string.h>

void
sl_parse_error(sl_parse_state_t* ps, char* message)
{
    SLVAL err = sl_make_cstring(ps->vm, message);
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, " in "));
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, (char*)ps->filename));
    err = sl_string_concat(ps->vm, err, sl_make_cstring(ps->vm, ", line "));
    err = sl_string_concat(ps->vm, err, sl_to_s(ps->vm, sl_make_int(ps->vm, ps->line)));
    sl_throw(ps->vm, sl_make_error2(ps->vm, ps->vm->lib.SyntaxError, err));
}

int
sl_node_is_lval(sl_node_base_t* node)
{
    size_t i = 0;
    sl_node_array_t* ary;
    switch(node->type) {
        case SL_NODE_VAR:    return 1;
        case SL_NODE_IVAR:   return 1;
        case SL_NODE_CVAR:   return 1;
        case SL_NODE_CONST:  return 1;
        case SL_NODE_GLOBAL: return 1;
        case SL_NODE_SEND:   return ((sl_node_send_t*)node)->arg_count == 0;
        case SL_NODE_ARRAY:
            ary = (sl_node_array_t*)node;
            for(i = 0; i < ary->node_count; i++) {
                if(!sl_node_is_lval(ary->nodes[i])) {
                    return 0;
                }
            }
            return 1;
        default:
            return 0;
    }
}

static sl_node_base_t*
sl_make_node(sl_parse_state_t* ps, sl_node_type_t type, size_t size)
{
    sl_node_base_t* node = sl_alloc(ps->vm->arena, size);
    node->type = type;
    return node;
}

sl_node_base_t*
sl_make_singleton_node(sl_parse_state_t* ps, sl_node_type_t type)
{
    return sl_make_node(ps, type, sizeof(sl_node_base_t));
}

#define MAKE_NODE(t, type, block) do { \
        type* node = (type*)sl_make_node(ps, (t), sizeof(type)); \
        ((sl_node_base_t*)node)->line = ps->line; \
        block; \
        return (sl_node_base_t*)node; \
    } while(0)

sl_node_seq_t*
sl_make_seq_node(sl_parse_state_t* ps)
{
    sl_node_seq_t* node = (sl_node_seq_t*)
        sl_make_node(ps, SL_NODE_SEQ, sizeof(sl_node_seq_t));
    node->node_capacity = 2;
    node->node_count = 0;
    node->nodes = (sl_node_base_t**)sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * node->node_capacity);
    return node;
}

void
sl_seq_node_append(sl_parse_state_t* ps, sl_node_seq_t* seq, sl_node_base_t* node)
{
    if(seq->node_count + 1 >= seq->node_capacity) {
        seq->node_capacity *= 2;
        seq->nodes = (sl_node_base_t**)sl_realloc(ps->vm->arena, seq->nodes, sizeof(sl_node_base_t*) * seq->node_capacity);
    }
    seq->nodes[seq->node_count++] = node;
}

sl_node_base_t*
sl_make_raw_node(sl_parse_state_t* ps, sl_token_t* token)
{
    MAKE_NODE(SL_NODE_RAW, sl_node_raw_t, {
        node->string = sl_make_string(ps->vm, token->as.str.buff, token->as.str.len);
    });
}

sl_node_base_t*
sl_make_echo_node(sl_parse_state_t* ps, sl_node_base_t* expr)
{
    MAKE_NODE(SL_NODE_ECHO, sl_node_echo_t, {
        node->expr = expr;
    });
}

sl_node_base_t*
sl_make_raw_echo_node(sl_parse_state_t* ps, sl_node_base_t* expr)
{
    MAKE_NODE(SL_NODE_ECHO_RAW, sl_node_echo_t, {
        node->expr = expr;
    });
}

sl_node_base_t*
sl_make_binary_node(sl_parse_state_t* ps, sl_node_base_t* left, sl_node_base_t* right, sl_node_type_t type)
{
    MAKE_NODE(type, sl_node_binary_t, {
        node->left = left;
        node->right = right;
    });
}

sl_node_base_t*
sl_make_unary_node(sl_parse_state_t* ps, sl_node_base_t* expr, sl_node_type_t type)
{
    MAKE_NODE(type, sl_node_unary_t, {
        node->expr = expr;
    });
}

sl_node_base_t*
sl_make_immediate_node(sl_parse_state_t* ps, SLVAL val)
{
    MAKE_NODE(SL_NODE_IMMEDIATE, sl_node_immediate_t, {
        node->value = val;
    });
}

sl_node_base_t*
sl_make_send_node(sl_parse_state_t* ps, sl_node_base_t* recv, SLVAL id, size_t argc, sl_node_base_t** argv)
{
    MAKE_NODE(SL_NODE_SEND, sl_node_send_t, {
        node->file = ps->filename;
        node->recv = recv;
        node->id = id;
        node->arg_count = argc;
        node->args = sl_alloc(ps->vm->arena, sizeof(sl_node_base_t*) * argc);
        memcpy(node->args, argv, sizeof(sl_node_base_t*) * argc);
    });
}

sl_node_base_t*
sl_make_var_node(sl_parse_state_t* ps, sl_node_type_t type, SLVAL id)
{
    MAKE_NODE(type, sl_node_var_t, {
        node->name = (sl_string_t*)sl_get_ptr(sl_expect(ps->vm, id, ps->vm->lib.String));
    });
}

sl_node_base_t*
sl_make_const_node(sl_parse_state_t* ps, sl_node_base_t* obj, SLVAL id)
{
    MAKE_NODE(SL_NODE_CONST, sl_node_const_t, {
        node->obj = obj;
        node->id = id;
    });
}

sl_node_base_t*
sl_make_if_node(sl_parse_state_t* ps, sl_node_base_t* cond, sl_node_base_t* if_true, sl_node_base_t* if_false)
{
    MAKE_NODE(SL_NODE_IF, sl_node_if_t, {
        node->condition = cond;
        node->body = if_true;
        node->else_body = if_false;
    });
}

sl_node_base_t*
sl_make_while_node(sl_parse_state_t* ps, sl_node_base_t* expr, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_WHILE, sl_node_while_t, {
        node->expr = expr;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_for_node(sl_parse_state_t* ps, sl_node_base_t* lval, sl_node_base_t* expr, sl_node_base_t* body, sl_node_base_t* else_body)
{
    MAKE_NODE(SL_NODE_FOR, sl_node_for_t, {
        node->lval = lval;
        node->expr = expr;
        node->body = body;
        node->else_body = else_body;
    });
}

sl_node_base_t*
sl_make_class_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* extends, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_CLASS, sl_node_class_t, {
        sl_expect(ps->vm, name, ps->vm->lib.String);
        node->name = name;
        node->extends = extends;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_def_node(sl_parse_state_t* ps, SLVAL name, sl_node_base_t* on, size_t arg_count, sl_string_t** args, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_DEF, sl_node_def_t, {
        sl_expect(ps->vm, name, ps->vm->lib.String);
        node->name = name;
        node->on = on;
        node->args = args;
        node->arg_count = arg_count;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_lambda_node(sl_parse_state_t* ps, size_t arg_count, sl_string_t** args, sl_node_base_t* body)
{
    MAKE_NODE(SL_NODE_LAMBDA, sl_node_lambda_t, {
        node->args = args;
        node->arg_count = arg_count;
        node->body = body;
    });
}

sl_node_base_t*
sl_make_try_node(sl_parse_state_t* ps, sl_node_base_t* body, sl_node_base_t* lval, sl_node_base_t* catch_body)
{
    MAKE_NODE(SL_NODE_TRY, sl_node_try_t, {
        node->body = body;
        node->lval = lval;
        node->catch_body = catch_body;
    });
}

sl_node_base_t*
sl_make_self_node(sl_parse_state_t* ps)
{
    MAKE_NODE(SL_NODE_SELF, sl_node_base_t, {});
}

sl_node_base_t*
sl_make_assign_var_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_VAR, sl_node_assign_var_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_ivar_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_IVAR, sl_node_assign_ivar_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_cvar_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_CVAR, sl_node_assign_cvar_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_global_node(sl_parse_state_t* ps, sl_node_var_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_GLOBAL, sl_node_assign_var_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_const_node(sl_parse_state_t* ps, sl_node_const_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_CONST, sl_node_assign_const_t, {
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_assign_array_node(sl_parse_state_t* ps, sl_node_array_t* lval, sl_node_base_t* rval)
{
    MAKE_NODE(SL_NODE_ASSIGN_ARRAY, sl_node_assign_array_t, {
        if(!sl_node_is_lval((sl_node_base_t*)lval)) {
            sl_parse_error(ps, "Non-assignable in array assignment");
        }
        node->lval = lval;
        node->rval = rval;
    });
}

sl_node_base_t*
sl_make_array_node(sl_parse_state_t* ps, size_t node_count, sl_node_base_t** nodes)
{
    MAKE_NODE(SL_NODE_ARRAY, sl_node_array_t, {
        node->node_count = node_count;
        node->nodes = nodes;
    });
}

sl_node_base_t*
sl_make_dict_node(sl_parse_state_t* ps, size_t node_count, sl_node_base_t** keys, sl_node_base_t** vals)
{
    MAKE_NODE(SL_NODE_DICT, sl_node_dict_t, {
        node->node_count = node_count;
        node->keys = keys;
        node->vals = vals;
    });
}

sl_node_base_t*
sl_make_range_node(sl_parse_state_t* ps, sl_node_base_t* left, sl_node_base_t* right, int exclusive)
{
    MAKE_NODE(SL_NODE_RANGE, sl_node_range_t, {
        node->left = left;
        node->right = right;
        node->exclusive = exclusive;
    });
}
