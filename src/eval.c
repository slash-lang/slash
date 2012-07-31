#include "eval.h"
#include "object.h"
#include "string.h"
#include "class.h"
#include "lex.h"
#include "parse.h"
#include "method.h"
#include "lib/array.h"
#include "lib/response.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>

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
    uint8_t* src;
    size_t file_size;
    size_t token_count;
    sl_token_t* tokens;
    sl_node_base_t* ast;
    sl_eval_ctx_t* ctx = sl_make_eval_ctx(vm);
    SLVAL err;
    
    if(!f) {
        err = sl_make_cstring(vm, "Could not load file: ");
        err = sl_string_concat(vm, err, sl_make_cstring(vm, (char*)filename));
        sl_throw(vm, sl_make_error2(vm, vm->lib.Error, err));
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

static void
set_lval(sl_node_base_t* node, sl_eval_ctx_t* ctx, SLVAL val);

static void
assign_seq(sl_node_seq_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    size_t i;
    if(!sl_is_a(ctx->vm, val, ctx->vm->lib.Array)) {
        set_lval(node->nodes[0], ctx, val);
        for(i = 1; i < node->node_count; i++) {
            set_lval(node->nodes[i], ctx, ctx->vm->lib.nil);
        }
    } else {
        for(i = 0; i < node->node_count; i++) {
            set_lval(node->nodes[i], ctx, sl_array_get(ctx->vm, val, i));
        }
    }
}

SLVAL
sl_eval_assign_seq(sl_node_assign_seq_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->rval->eval(node->rval, ctx);
    assign_seq(node->lval, ctx, val);
    return val;
}

static void
assign_var(sl_node_var_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    sl_eval_ctx_t* octx = ctx;
    while(ctx) {
        if(st_lookup(ctx->vars, (st_data_t)node->name, NULL)) {
            st_insert(ctx->vars, (st_data_t)node->name, (st_data_t)sl_get_ptr(val));
            return;
        }
        ctx = ctx->parent;
    }
    st_insert(octx->vars, (st_data_t)node->name, (st_data_t)sl_get_ptr(val));
}

SLVAL
sl_eval_assign_var(sl_node_assign_var_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->rval->eval(node->rval, ctx);
    assign_var(node->lval, ctx, val);
    return val;
}

static void
assign_ivar(sl_node_var_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    sl_set_ivar(ctx->vm, ctx->self, node->name, val);
}

SLVAL
sl_eval_assign_ivar(sl_node_assign_ivar_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->rval->eval(node->rval, ctx);
    assign_ivar(node->lval, ctx, val);
    return val;
}

static void
assign_cvar(sl_node_var_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    sl_set_cvar(ctx->vm, ctx->self, node->name, val);
}

SLVAL
sl_eval_assign_cvar(sl_node_assign_cvar_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->rval->eval(node->rval, ctx);
    assign_cvar(node->lval, ctx, val);
    return val;
}

static void
assign_const(sl_node_const_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    SLVAL obj;
    if(node->obj) {
        obj = node->obj->eval(node->obj, ctx);
    } else {
        obj = ctx->self;
        if(!sl_is_a(ctx->vm, obj, ctx->vm->lib.Class)) {
            obj = sl_class_of(ctx->vm, obj);
        }
    }
    sl_class_set_const2(ctx->vm, obj, node->id, val);
}

SLVAL
sl_eval_assign_const(sl_node_assign_const_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL val = node->rval->eval(node->rval, ctx);
    assign_const(node->lval, ctx, val);
    return val;
}

static void
assign_send(sl_node_send_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    sl_node_send_t n;
    sl_node_immediate_t imm;
    sl_node_base_t** args;
    imm.base.eval = (sl_node_eval_fn_t)sl_eval_immediate;
    imm.base.type = SL_NODE_IMMEDIATE;
    imm.value = val;
    memcpy(&n, node, sizeof(n));
    n.id = sl_string_concat(ctx->vm, n.id, sl_make_cstring(ctx->vm, "="));
    n.line = node->line;
    args = alloca(sizeof(sl_node_base_t*) * (n.arg_count + 1));
    args[n.arg_count++] = &imm.base;
    sl_eval_send(&n, ctx);
}

static void
set_lval(sl_node_base_t* node, sl_eval_ctx_t* ctx, SLVAL val)
{
    switch(node->type) {
        case SL_NODE_SEQ:
            assign_seq((sl_node_seq_t*)node, ctx, val);
            break;
        case SL_NODE_VAR:
            assign_var((sl_node_var_t*)node, ctx, val);
            break;
        case SL_NODE_IVAR:
            assign_ivar((sl_node_var_t*)node, ctx, val);
            break;
        case SL_NODE_CVAR:
            assign_cvar((sl_node_var_t*)node, ctx, val);
            break;
        case SL_NODE_CONST:
            assign_const((sl_node_const_t*)node, ctx, val);
            break;
        case SL_NODE_SEND:
            assign_send((sl_node_send_t*)node, ctx, val);
            break;
        default:
            sl_throw_message2(ctx->vm, ctx->vm->lib.TypeError, "Invalid lval in assignment");
    }
}

SLVAL
sl_eval_seq(sl_node_seq_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL retn = ctx->vm->lib.nil;
    size_t i;
    for(i = 0; i < node->node_count; i++) {
        retn = node->nodes[i]->eval(node->nodes[i], ctx);
    }
    return retn;
}

SLVAL
sl_eval_raw(sl_node_raw_t* node, sl_eval_ctx_t* ctx)
{
    sl_response_write(ctx->vm, node->string);
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_echo(sl_node_echo_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL str = sl_to_s(ctx->vm, node->expr->eval(node->expr, ctx));
    sl_response_write(ctx->vm, sl_string_html_escape(ctx->vm, str));
    return ctx->vm->lib.nil;
}

SLVAL
sl_eval_echo_raw(sl_node_echo_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL str = sl_to_s(ctx->vm, node->expr->eval(node->expr, ctx));
    sl_response_write(ctx->vm, str);
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
sl_eval_immediate(sl_node_immediate_t* node, sl_eval_ctx_t* ctx)
{
    (void)ctx;
    return node->value;
}

SLVAL
sl_eval_send(sl_node_send_t* node, sl_eval_ctx_t* ctx)
{
    sl_catch_frame_t frame;
    sl_vm_t* vm = ctx->vm;
    SLVAL recv = node->recv->eval(node->recv, ctx);
    SLVAL* args = alloca(sizeof(SLVAL) * node->arg_count);
    SLVAL ret, err;
    size_t i;
    for(i = 0; i < node->arg_count; i++) {
        /* @TODO splat would go here... */
        args[i] = node->args[i]->eval(node->args[i], ctx);
    }
    SL_TRY(frame, SL_UNWIND_EXCEPTION, {
        ret = sl_send2(ctx->vm, recv, node->id, node->arg_count, args);
    }, err, {
        sl_error_add_frame(vm, err, recv, node->id, sl_make_cstring(ctx->vm, (char*)node->file), sl_make_int(ctx->vm, node->line));
        sl_rethrow(vm, &frame);
        return err; /* never reached */
    });
    return ret;
}

SLVAL
sl_eval_const(sl_node_const_t* node, sl_eval_ctx_t* ctx)
{
    sl_vm_t* vm = ctx->vm;
    SLVAL self = ctx->self;
    if(node->obj) {
        return sl_class_get_const2(ctx->vm, node->obj->eval(node->obj, ctx), node->id);
    } else {
        while(ctx) {
            if(sl_class_has_const2(vm, ctx->self, node->id)) {
                return sl_class_get_const2(vm, ctx->self, node->id);
            }
            ctx = ctx->parent;
        }
        /* not found */
        return sl_class_get_const2(vm, self, node->id);
    }
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
sl_eval_for(sl_node_for_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL enumerable = node->expr->eval(node->expr, ctx);
    SLVAL enumerator = sl_send(ctx->vm, enumerable, "enumerate", 0, NULL);
    SLVAL val;
    int iterated = 0;
    while(sl_is_truthy(sl_send(ctx->vm, enumerator, "next", 0, NULL))) {
        val = sl_send(ctx->vm, enumerator, "current", 0, NULL);
        set_lval(node->lval, ctx, val);
        iterated = 1;
        node->body->eval(node->body, ctx);
    }
    if(!iterated) {
        if(node->else_body) {
            node->else_body->eval(node->else_body, ctx);
        }
    }
    return enumerable;
}

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
    SLVAL on, method, mklass;
    void(*definer)(sl_vm_t*, SLVAL, SLVAL, SLVAL) = NULL;
    if(node->on) {
        on = node->on->eval(node->on, ctx);
        mklass = on;
        if(!sl_is_a(ctx->vm, mklass, ctx->vm->lib.Class)) {
            mklass = sl_class_of(ctx->vm, mklass);
        }
        definer = sl_define_singleton_method3;
    } else {
        on = ctx->self;
        if(!sl_is_a(ctx->vm, on, ctx->vm->lib.Class)) {
            on = sl_class_of(ctx->vm, on);
        }
        mklass = on;
        definer = sl_define_method3;
    }
    method = sl_make_method(ctx->vm, mklass, node->name, node->arg_count,
        node->arg_count, node->args, node->body, ctx);
    definer(ctx->vm, on, node->name, method);
    return method;
}

SLVAL
sl_eval_try(sl_node_try_t* node, sl_eval_ctx_t* ctx)
{
    sl_vm_t* vm = ctx->vm;
    sl_catch_frame_t frame;
    SLVAL err, ret;
    SL_TRY(frame, SL_UNWIND_EXCEPTION, {
        ret = node->body->eval(node->body, ctx);
    }, err, {
        if(node->lval) {
            set_lval(node->lval, ctx, err);
        }
        ret = node->catch_body->eval(node->catch_body, ctx);
    });
    return ret;
}

SLVAL
sl_eval_self(sl_node_base_t* node, sl_eval_ctx_t* ctx)
{
    (void)node;
    return ctx->self;
}

SLVAL
sl_eval_array(sl_node_array_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL* items = GC_MALLOC(sizeof(SLVAL) * node->node_count);
    size_t i;
    for(i = 0; i < node->node_count; i++) {
        items[i] = node->nodes[i]->eval(node->nodes[i], ctx);
    }
    return sl_make_array(ctx->vm, node->node_count, items);
}

SLVAL
sl_eval_dict(sl_node_dict_t* node, sl_eval_ctx_t* ctx)
{
    SLVAL dict = sl_make_dict(ctx->vm, 0, NULL);
    size_t i;
    for(i = 0; i < node->node_count; i++) {
        sl_dict_set(ctx->vm, dict, node->keys[i]->eval(node->keys[i], ctx), node->vals[i]->eval(node->vals[i], ctx));
    }
    return dict;
}

SLVAL
sl_eval_return(sl_node_unary_t* node, sl_eval_ctx_t* ctx)
{
    sl_return(ctx->vm, node->expr->eval(node->expr, ctx));
    return ctx->vm->lib.nil; /* never reached */
}
