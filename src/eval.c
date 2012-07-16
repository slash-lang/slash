#include "eval.h"
#include "object.h"
#include <gc.h>
#include <string.h>

SLVAL
sl_eval_seq(sl_node_seq_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL retn;
    size_t i;
    for(i = 0; i < node->node_count; i++) {
        retn = node->nodes[i]->eval(node->nodes[i], ctx);
    }
    return retn;
}

SLVAL
sl_eval_raw(sl_node_raw_t* node, sl_eval_ctx_t* ctx)
{
    sl_string_t* str = (sl_string_t*)sl_get_ptr(node->string);
    ctx->vm->output(ctx->vm, (char*)str->buff, str->buff_len);
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_echo(sl_node_echo_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL strv = sl_to_s(ctx->vm, node->expr->eval(node->expr, ctx));
    sl_string_t* str = (sl_string_t*)sl_get_ptr(strv);
    size_t out_cap = 32;
    size_t out_len = 0;
    size_t str_i;
    uint8_t* out = GC_MALLOC(out_cap);
    for(str_i = 0; str_i < str->buff_len; str_i++) {
        if(out_len + 8 >= out_cap) {
            out_cap *= 2;
            out = GC_REALLOC(out, out_cap);
        }
        if(str->buff[str_i] == '<') {
            memcpy(out + out_len, "&lt;", 4);
            out_len += 4;
        } else if(str->buff[str_i] == '>') {
            memcpy(out + out_len, "&gt;", 4);
            out_len += 4;
        } else if(str->buff[str_i] == '"') {
            memcpy(out + out_len, "&quot;", 6);
            out_len += 6;
        } else if(str->buff[str_i] == '\'') {
            memcpy(out + out_len, "&#039;", 6);
            out_len += 6;
        } else if(str->buff[str_i] == '&') {
            memcpy(out + out_len, "&amp;", 5);
            out_len += 5;
        } else {
            out[out_len++] = str->buff[str_i];
        }
    }
    ctx->vm->output(ctx->vm, (char*)out, out_len);
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_echo_raw(sl_node_echo_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL strv = sl_to_s(ctx->vm, node->expr->eval(node->expr, ctx));
    sl_string_t* str = (sl_string_t*)sl_get_ptr(strv);
    ctx->vm->output(ctx->vm, (char*)str->buff, str->buff_len);
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_and(sl_node_binary_t* node, sl_eval_ctx_t* ctx)
{
    (void)node;
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_or(sl_node_binary_t* node, sl_eval_ctx_t* ctx)
{
    (void)node;
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_not(sl_node_unary_t* node, sl_eval_ctx_t* ctx)
{
    (void)node;
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_assign(sl_node_binary_t* node, sl_eval_ctx_t* ctx)
{
    (void)node;
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_immediate(sl_node_immediate_t* node, sl_eval_ctx_t* ctx)
{
    (void)ctx;
    return node->value;
}

SLVAL
sl_eval_send(sl_node_send_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL recv = node->recv->eval(node->recv, ctx);
    SLVAL* args = GC_MALLOC(sizeof(SLVAL) * node->arg_count);
    size_t i;
    for(i = 0; i < node->arg_count; i++) {
        /* @TODO splat would go here... */
        args[i] = node->args[i]->eval(node->args[i], ctx);
    }
    return sl_send2(ctx->vm, recv, sl_make_ptr((sl_object_t*)node->id), node->arg_count, args);
}
