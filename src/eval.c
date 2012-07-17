#include "eval.h"
#include "object.h"
#include "string.h"
#include "class.h"
#include "lex.h"
#include "parse.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>

sl_eval_ctx_t* sl_make_eval_ctx(sl_vm_t* vm)
{
    sl_eval_ctx_t* ctx = GC_MALLOC(sizeof(sl_eval_ctx_t));
    ctx->vm = vm;
    ctx->self = vm->lib.Object;
    ctx->vars = st_init_table(&sl_string_hash_type);
    ctx->parent = NULL;
    return ctx;
}

SLVAL
sl_do_file(sl_vm_t* vm, uint8_t* filename)
{
    FILE* f = fopen((char*)filename, "rb");
    char buff[1024];
    uint8_t* src;
    size_t file_size;
    size_t token_count;
    sl_token_t* tokens;
    sl_node_base_t* ast;
    sl_eval_ctx_t* ctx = sl_make_eval_ctx(vm);
    
    if(!f) {
        snprintf(buff, 1023, "Could not load file: %s", filename);
        sl_throw_message2(vm, vm->lib.Error, buff);
    }
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    src = GC_MALLOC(file_size);
    fread(src, file_size, 1, f);
    fclose(f);
    
    tokens = sl_lex(vm, filename, src, file_size, &token_count);
    ast = sl_parse(vm, tokens, token_count, filename);
    return ast->eval(ast, ctx);
}

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
    sl_string_t* str;
    strv = sl_string_html_escape(ctx->vm, strv);
    str = (sl_string_t*)sl_get_ptr(strv);
    ctx->vm->output(ctx->vm, (char*)str->buff, str->buff_len);
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
    SLVAL val = node->left->eval(node->left, ctx);
    if(sl_is_truthy(val)) {
        return node->right->eval(node->right, ctx);
    }
    return val;
}

SLVAL
sl_eval_or(sl_node_binary_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->left->eval(node->left, ctx);
    if(sl_is_truthy(val)) {
        return val;
    }
    return node->right->eval(node->right, ctx);
}

SLVAL
sl_eval_not(sl_node_unary_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->expr->eval(node->expr, ctx);
    if(sl_is_truthy(val)) {
        return ctx->vm->lib._false;
    } else {
        return ctx->vm->lib._true;
    }
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

SLVAL
sl_eval_constant(sl_node_var_t* node, sl_eval_ctx_t* ctx)
{
    /* @TODO look up constants in nested scopes */
    return sl_class_get_const2(ctx->vm, ctx->self, sl_make_ptr((sl_object_t*)node->name));
}

SLVAL
sl_eval_var(sl_node_var_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val;
    SLVAL err;
    sl_eval_ctx_t* octx = ctx;
    while(ctx) {
        if(st_lookup(ctx->vars, (st_data_t)node->name, (st_data_t*)&val)) {
            return val;
        }
        ctx = ctx->parent;
    }
    err = sl_make_cstring(octx->vm, "Undefined variable '");
    err = sl_string_concat(octx->vm, err, sl_make_ptr((sl_object_t*)node->name));
    err = sl_string_concat(octx->vm, err, sl_make_cstring(octx->vm, "'"));
    sl_throw(octx->vm, sl_make_error2(octx->vm, octx->vm->lib.NameError, err));
    return octx->vm->lib.nil;
}

SLVAL
sl_eval_ivar(sl_node_var_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_cvar(sl_node_var_t* node, sl_eval_ctx_t* ctx);
