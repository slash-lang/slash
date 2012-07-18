#include "eval.h"
#include "object.h"
#include "string.h"
#include "class.h"
#include "lex.h"
#include "parse.h"
#include "method.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>

sl_eval_ctx_t*
sl_make_eval_ctx(sl_vm_t* vm)
{
    sl_eval_ctx_t* ctx = GC_MALLOC(sizeof(sl_eval_ctx_t));
    ctx->vm = vm;
    ctx->self = vm->lib.Object;
    ctx->vars = st_init_table(&sl_string_hash_type);
    ctx->parent = NULL;
    return ctx;
}

sl_eval_ctx_t*
sl_close_eval_ctx(sl_vm_t* vm, sl_eval_ctx_t* ctx)
{
    sl_eval_ctx_t* subctx = sl_make_eval_ctx(vm);
    subctx->self = ctx->self;
    subctx->parent = ctx;
    return subctx;
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
    return sl_send2(ctx->vm, recv, node->id, node->arg_count, args);
}

SLVAL
sl_eval_const(sl_node_const_t* node, sl_eval_ctx_t* ctx)
{
    /* @TODO look up constants in nested scopes */
    return sl_class_get_const2(ctx->vm, node->obj->eval(node->obj, ctx), node->id);
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
sl_eval_ivar(sl_node_var_t* node, sl_eval_ctx_t* ctx)
{
    return sl_get_ivar(ctx->vm, ctx->self, node->name);
}

SLVAL
sl_eval_cvar(sl_node_var_t* node, sl_eval_ctx_t* ctx)
{
    return sl_get_cvar(ctx->vm, ctx->self, node->name);
}

SLVAL
sl_eval_if(sl_node_if_t* node, sl_eval_ctx_t* ctx)
{
    if(sl_is_truthy(node->condition->eval(node->condition, ctx))) {
        return node->body->eval(node->body, ctx);
    } else {
        if(node->else_body) {
            return node->else_body->eval(node->else_body, ctx);
        } else {
            return ctx->vm->lib.nil;
        }
    }
}

SLVAL
sl_eval_for(sl_node_for_t* node, sl_eval_ctx_t* ctx);

SLVAL
sl_eval_while(sl_node_while_t* node, sl_eval_ctx_t* ctx)
{
    while(sl_is_truthy(node->expr->eval(node->expr, ctx))) {
        node->body->eval(node->body, ctx);
    }
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_class(sl_node_class_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL klass, in, super;
    sl_eval_ctx_t* kctx;
    if(sl_class_has_const2(ctx->vm, ctx->self, node->name)) {
        klass = sl_class_get_const2(ctx->vm, ctx->self, node->name);
        sl_expect(ctx->vm, klass, ctx->vm->lib.Class);
        /* @TODO verify superclass is the same */
    } else {
        in = ctx->self;
        if(!sl_is_a(ctx->vm, in, ctx->vm->lib.Class)) {
            in = sl_class_of(ctx->vm, in);
        }
        if(node->extends != NULL) {
            super = node->extends->eval(node->extends, ctx);
        } else {
            super = ctx->vm->lib.Object;
        }
        klass = sl_define_class3(ctx->vm, node->name, super, in);
    }
    kctx = sl_close_eval_ctx(ctx->vm, ctx);
    kctx->self = klass;
    node->body->eval(node->body, kctx);
    return klass;
}

SLVAL
sl_eval_def(sl_node_def_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL on, method;
    void(*definer)(sl_vm_t*, SLVAL, SLVAL, SLVAL);
    if(node->on) {
        on = node->on->eval(node->on, ctx);
        definer = sl_define_singleton_method3;
    } else {
        on = ctx->self;
        if(!sl_is_a(ctx->vm, on, ctx->vm->lib.Class)) {
            on = sl_class_of(ctx->vm, on);
        }
        definer = sl_define_method3;
    }
    method = sl_make_method(ctx->vm, node->name, node->arg_count,
        node->arg_count, node->args, node->body, ctx);
    definer(ctx->vm, on, node->name, method);
    return method;
}
