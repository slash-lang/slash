#include <slash/compile.h>
#include <slash/string.h>
#include <slash/method.h>
#include <string.h>

#include "gen/opcode_enum.inc"

typedef struct fixup {
    struct fixup* next;
    size_t fixup;
}
fixup_t;

typedef struct next_last_frame {
    struct next_last_frame* prev;
    fixup_t* next_fixups;
    fixup_t* last_fixups;
    size_t try_catch_blocks;
}
next_last_frame_t;

typedef struct sl_compile_state {
    sl_vm_t* vm;
    sl_st_table_t* vars;
    struct sl_compile_state* parent;
    uint8_t* registers;
    sl_vm_section_t* section;
    next_last_frame_t* next_last_frames;
    int last_line;
    bool emitted_line_trace;
}
sl_compile_state_t;

static void
init_compile_state(sl_compile_state_t* cs, sl_vm_t* vm, sl_compile_state_t* parent, size_t init_registers)
{
    size_t i;
    cs->vm = vm;
    cs->vars = sl_st_init_table(vm, &sl_string_hash_type);
    cs->parent = parent;
    cs->last_line = 0;
    cs->section = sl_alloc(vm->arena, sizeof(sl_vm_section_t));
    if(parent) {
        cs->section->filename = parent->section->filename;
    }
    cs->section->max_registers = init_registers;
    cs->section->req_registers = 0;
    cs->section->arg_registers = 0;
    cs->section->insns_cap = 16;
    cs->section->insns_count = 0;
    cs->section->insns = sl_alloc(vm->arena, sizeof(sl_vm_insn_t) * cs->section->insns_cap);
    cs->section->can_stack_alloc_frame = true;
    cs->section->has_try_catch = false;
    cs->section->opt_skip = NULL;
    cs->registers = sl_alloc(vm->arena, cs->section->max_registers);
    for(i = 0; i < init_registers; i++) {
        cs->registers[i] = 1;
    }
    cs->next_last_frames = NULL;
    cs->emitted_line_trace = false;
}

static size_t
reg_alloc(sl_compile_state_t* cs)
{
    for(int i = 0; i < cs->section->max_registers; i++) {
        if(!cs->registers[i]) {
            cs->registers[i] = 1;
            return i;
        }
    }
    cs->section->max_registers++;
    cs->registers = sl_realloc(cs->vm->arena, cs->registers, cs->section->max_registers);
    cs->registers[cs->section->max_registers - 1] = 1;
    return cs->section->max_registers - 1;
}

static size_t
reg_alloc_block(sl_compile_state_t* cs, int count)
{
    if(count == 0) {
        return 0;
    }

    for(int i = 0; i + count < cs->section->max_registers + 1; i++) {
        if(cs->registers[i]) {
            continue_outer: continue;
        }
        for(int j = 0; j < count; j++) {
            if(cs->registers[i + j]) {
                i += j;
                goto continue_outer;
            }
        }
        for(int j = 0; j < count; j++) {
            cs->registers[i + j] = 1;
        }
        return i;
    }
    cs->section->max_registers += count;
    cs->registers = sl_realloc(cs->vm->arena, cs->registers, cs->section->max_registers);
    int i = cs->section->max_registers - count;
    for(int j = 0; j < count; j++) {
        cs->registers[i + j] = 1;
    }
    return i;
}

static void
reg_free(sl_compile_state_t* cs, size_t reg)
{
    cs->registers[reg] = 0;
}

static void
reg_free_block(sl_compile_state_t* cs, size_t reg, int count)
{
    for(int i = 0; i < count; i++) {
        reg_free(cs, reg + i);
    }
}

static sl_vm_insn_t*
insn_at_ip(sl_compile_state_t* cs, size_t ip)
{
    return &cs->section->insns[ip];
}

static size_t
_emit(sl_compile_state_t* cs, sl_vm_insn_t insn)
{
    if(cs->section->insns_count + 1 >= cs->section->insns_cap) {
        cs->section->insns_cap *= 2;
        cs->section->insns = sl_realloc(cs->vm->arena, cs->section->insns, sizeof(sl_vm_insn_t) * cs->section->insns_cap);
    }
    cs->section->insns[cs->section->insns_count] = insn;
    return cs->section->insns_count++;
}

#ifdef SL_HAS_COMPUTED_GOTO
extern void*
sl_vm_op_addresses[];

static size_t
_emit_opcode(sl_compile_state_t* cs, sl_vm_opcode_t opcode)
{
    sl_vm_insn_t insn;
    insn.threaded_opcode = sl_vm_op_addresses[opcode];
    cs->emitted_line_trace = opcode == SL_OP_LINE_TRACE;
    return _emit(cs, insn);
}
#else
static size_t
_emit_opcode(sl_compile_state_t* cs, sl_vm_opcode_t opcode)
{
    sl_vm_insn_t insn;
    cs->emitted_line_trace = opcode == SL_OP_LINE_TRACE;
    insn.opcode = opcode;
    return _emit(cs, insn);
}
#endif

#include "gen/compile_helper.inc"

static void
emit_send(sl_compile_state_t* cs, size_t recv, SLID id, size_t arg_base, size_t arg_size, size_t return_reg)
{
    sl_vm_inline_method_cache_t* imc = sl_alloc(cs->vm->arena, sizeof(sl_vm_inline_method_cache_t));
    imc->argc = arg_size;
    imc->id = id;
    imc->call = NULL;

    op_send(cs, recv, imc, arg_base, return_reg);
}

static void
emit_send_self(sl_compile_state_t* cs, SLID id, size_t arg_base, size_t arg_size, size_t return_reg)
{
    sl_vm_inline_method_cache_t* imc = sl_alloc(cs->vm->arena, sizeof(sl_vm_inline_method_cache_t));
    imc->argc = arg_size;
    imc->id = id;
    imc->call = NULL;

    op_send_self(cs, imc, arg_base, return_reg);
}

static void
compile_node(sl_compile_state_t* cs, sl_node_base_t* node, size_t dest);

static void
emit_assignment(sl_compile_state_t* cs, sl_node_base_t* lval, size_t reg)
{
    sl_node_assign_var_t a_var;
    sl_node_send_t send;

    size_t dest_reg = reg_alloc(cs);

    sl_node__register_t node;
    node.base.type = SL_NODE__REGISTER;
    node.base.line = 0;
    node.reg = reg;

    switch(lval->type) {
        case SL_NODE_VAR:    a_var.base.type = SL_NODE_ASSIGN_VAR;    break;
        case SL_NODE_IVAR:   a_var.base.type = SL_NODE_ASSIGN_IVAR;   break;
        case SL_NODE_CVAR:   a_var.base.type = SL_NODE_ASSIGN_CVAR;   break;
        case SL_NODE_CONST:  a_var.base.type = SL_NODE_ASSIGN_CONST;  break;
        case SL_NODE_ARRAY:  a_var.base.type = SL_NODE_ASSIGN_ARRAY;  break;
        case SL_NODE_SEND:
            /* special case that turns a.b = 1 into a.send("b=", 1) */
            /* this is separate to the other method of handling send assignments
               which also handles compound assignments. */
            memcpy(&send, lval, sizeof(sl_node_send_t));
            send.id = sl_id_make_setter(cs->vm, send.id);
            sl_node_base_t** args = sl_alloc(cs->vm->arena, sizeof(sl_node_base_t*) * (send.arg_count + 1));
            memcpy(args, send.args, sizeof(sl_node_base_t*) * send.arg_count);
            args[send.arg_count++] = (sl_node_base_t*)&node;
            send.args = args;
            compile_node(cs, (sl_node_base_t*)&send, dest_reg);
            reg_free(cs, dest_reg);
            return;
        default: {
            SLVAL err = sl_make_cstring(cs->vm, "Invalid lval in assignment");
            err = sl_make_error2(cs->vm, cs->vm->lib.SyntaxError, err);
            sl_error_add_frame(cs->vm, err, sl_make_cstring(cs->vm, "<compiler>"), sl_make_cstring(cs->vm, (char*)cs->section->filename), sl_make_int(cs->vm, lval->line));
            sl_throw(cs->vm, err);
        }
    }

    a_var.base.line = 0;
    a_var.lval = (void*)lval;
    a_var.rval = (sl_node_base_t*)&node;
    compile_node(cs, (sl_node_base_t*)&a_var, dest_reg);

    reg_free(cs, dest_reg);
}

#define NODE(type, name) static void compile_##name(sl_compile_state_t* cs, type* node, size_t dest)

NODE(sl_node_seq_t, seq)
{
    size_t i;
    for(i = 0; i < node->node_count; i++) {
        /*  we only care about the result of the last node so we'll write all
            results to the same output register we were given */
        compile_node(cs, node->nodes[i], dest);
    }
    if(node->node_count == 0) {
        op_immediate(cs, cs->vm->lib.nil, dest);
    }
}

NODE(sl_node_raw_t, raw)
{
    op_raw(cs, node->string);
    (void)dest;
}

NODE(sl_node_echo_t, echo)
{
    compile_node(cs, node->expr, dest);
    op_echo(cs, dest);
}

NODE(sl_node_echo_t, echo_raw)
{
    compile_node(cs, node->expr, dest);
    op_echo_raw(cs, dest);
}

static void
mark_upper_scopes_as_closure_unsafe(sl_compile_state_t* cs, size_t count)
{
    while(cs->parent && count--) {
        cs->parent->section->can_stack_alloc_frame = 0;
        cs = cs->parent;
    }
}

NODE(sl_node_var_t, var)
{
    size_t frame;
    sl_compile_state_t* xcs = cs;
    size_t index = 0xCAFE;
    SLVAL err;
    frame = 0;
    while(xcs) {
        if(sl_st_lookup(xcs->vars, (sl_st_data_t)node->name, (sl_st_data_t*)&index)) {
            if(frame == 0) {
                op_mov(cs, index, dest);
            } else {
                op_get_outer(cs, frame, index, dest);
                mark_upper_scopes_as_closure_unsafe(cs, frame);
            }
            return;
        }
        xcs = xcs->parent;
        frame++;
    }
    err = sl_make_formatted_string(cs->vm, "Undefined variable %QV", node->name);
    err = sl_make_error2(cs->vm, cs->vm->lib.NameError, err);
    sl_error_add_frame(cs->vm, err, sl_make_cstring(cs->vm, "<compiler>"), sl_make_cstring(cs->vm, (char*)cs->section->filename), sl_make_int(cs->vm, node->base.line));
    sl_throw(cs->vm, err);
}

NODE(sl_node_var_t, ivar)
{
    SLID id = sl_intern2(cs->vm, sl_make_ptr((sl_object_t*)node->name));
    op_get_ivar(cs, id, dest);
}

NODE(sl_node_var_t, cvar)
{
    SLID id = sl_intern2(cs->vm, sl_make_ptr((sl_object_t*)node->name));
    op_get_cvar(cs, id, dest);
}


NODE(sl_node_immediate_t, immediate)
{
    op_immediate(cs, node->value, dest);
}

NODE(sl_node_interp_string_t, interp_string)
{
    size_t base_reg = reg_alloc_block(cs, node->components_count);
    for(size_t i = 0; i < node->components_count; i++) {
        compile_node(cs, node->components[i], base_reg + i);
    }
    op_build_string(cs, base_reg, node->components_count, dest);
    reg_free_block(cs, base_reg, node->components_count);
}

NODE(sl_node_base_t, self)
{
    op_self(cs, dest);
    (void)node;
}

NODE(sl_node_def_t, def)
{
    sl_compile_state_t sub_cs;
    size_t i, on_reg;
    size_t init_regs = 1 + node->req_arg_count + node->opt_arg_count;
    if(node->rest_arg) {
        init_regs++;
    }
    init_compile_state(&sub_cs, cs->vm, cs, init_regs);
    for(i = 0; i < node->req_arg_count; i++) {
        sl_st_insert(sub_cs.vars, (sl_st_data_t)node->req_args[i], (sl_st_data_t)(i + 1));
    }
    for(i = 0; i < node->opt_arg_count; i++) {
        sl_st_insert(sub_cs.vars, (sl_st_data_t)node->opt_args[i].name, (sl_st_data_t)(node->req_arg_count + i + 1));
    }
    if(node->rest_arg) {
        sl_st_insert(sub_cs.vars, (sl_st_data_t)node->rest_arg, (sl_st_data_t)(node->req_arg_count + node->opt_arg_count + 1));
        sub_cs.section->has_extra_rest_arg = 1;
    }
    sub_cs.section->name = node->name;
    sub_cs.section->req_registers = node->req_arg_count;
    sub_cs.section->arg_registers = node->req_arg_count + node->opt_arg_count;
    sub_cs.section->opt_skip = sl_alloc(cs->vm->arena, sizeof(size_t) * (node->opt_arg_count + 1));

    for(i = 0; i < node->opt_arg_count; i++) {
        sub_cs.section->opt_skip[i] = sub_cs.section->insns_count;
        compile_node(&sub_cs, node->opt_args[i].default_value, node->req_arg_count + i + 1);
    }
    sub_cs.section->opt_skip[node->opt_arg_count] = sub_cs.section->insns_count;

    compile_node(&sub_cs, node->body, 0);
    op_return(&sub_cs, 0);

    if(node->on) {
        on_reg = reg_alloc(cs);
        compile_node(cs, node->on, on_reg);
        op_define_on(cs, on_reg, node->name, node->doc, sub_cs.section, dest);
        reg_free(cs, on_reg);
    } else {
        op_define(cs, node->name, node->doc, sub_cs.section, dest);
    }
}

NODE(sl_node_lambda_t, lambda)
{
    sl_compile_state_t sub_cs;
    size_t i;
    init_compile_state(&sub_cs, cs->vm, cs, node->arg_count + 1);
    for(i = 0; i < node->arg_count; i++) {
        sl_st_insert(sub_cs.vars, (sl_st_data_t)node->args[i], (sl_st_data_t)(i + 1));
    }
    sub_cs.section->req_registers = node->arg_count;
    sub_cs.section->arg_registers = node->arg_count;
    sub_cs.section->name = sl_intern(cs->vm, "<lambda>");
    compile_node(&sub_cs, node->body, 0);
    op_return(&sub_cs, 0);
    op_lambda(cs, sub_cs.section, dest);
}

NODE(sl_node_try_t, try)
{
    size_t catch_fixup, after_fixup;

    cs->section->has_try_catch = true;

    catch_fixup = op_try(cs, 0xdeadbeef) + 1;

    if(cs->next_last_frames) {
        cs->next_last_frames->try_catch_blocks++;
    }

    compile_node(cs, node->body, dest);

    if(cs->next_last_frames) {
        cs->next_last_frames->try_catch_blocks--;
    }

    op_end_try(cs);

    after_fixup = op_jump(cs, 0xdeadbeef) + 1;

    insn_at_ip(cs, catch_fixup)->uint = cs->section->insns_count;

    op_catch(cs, dest);
    op_end_try(cs);

    if(node->lval) {
        emit_assignment(cs, node->lval, dest);
    }
    compile_node(cs, node->catch_body, dest);

    insn_at_ip(cs, after_fixup)->uint = cs->section->insns_count;
}

NODE(sl_node_class_t, class)
{
    sl_compile_state_t sub_cs;

    if(node->extends) {
        compile_node(cs, node->extends, dest);
    } else {
        op_immediate(cs, cs->vm->lib.Object, dest);
    }

    init_compile_state(&sub_cs, cs->vm, cs, 1);
    sub_cs.section->name = sl_intern2(cs->vm,
        sl_string_concat(cs->vm, sl_make_cstring(cs->vm, "class "), sl_id_to_string(cs->vm, node->name)));
    compile_node(&sub_cs, node->body, 0);
    op_return(&sub_cs, 0);
    op_class(cs, node->name, node->doc, dest, sub_cs.section, dest);
}

NODE(sl_node_if_t, if)
{
    /* emit a jump over the true branch, keeping a pointer to fixup later */
    compile_node(cs, node->condition, dest);
    size_t fixup_1 = op_jump_unless(cs, 0x0000CAFE, dest) + 1;

    /* true branch */
    compile_node(cs, node->body, dest);

    size_t fixup_2 = op_jump(cs, 0x0000CAFE) + 1;

    insn_at_ip(cs, fixup_1)->uint = cs->section->insns_count;

    if(node->else_body) {
        compile_node(cs, node->else_body, dest);
    } else {
        op_immediate(cs, cs->vm->lib.nil, dest);
    }

    insn_at_ip(cs, fixup_2)->uint = cs->section->insns_count;
}

NODE(sl_node_switch_t, switch)
{
    compile_node(cs, node->value, dest);
    size_t* fixups = sl_alloc_buffer(cs->vm->arena, sizeof(size_t) * node->case_count);
    for(size_t i = 0; i < node->case_count; i++) {
        size_t cmp_reg = reg_alloc(cs);
        compile_node(cs, node->cases[i].value, cmp_reg);
        emit_send(cs, cmp_reg, sl_intern(cs->vm, "=="), dest, 1, cmp_reg);
        size_t temp_fixup = op_jump_unless(cs, 0x0000CAFE, cmp_reg) + 1;
        reg_free(cs, cmp_reg);
        compile_node(cs, node->cases[i].body, dest);
        fixups[i] = op_jump(cs, 0x0000CAFE) + 1;
        insn_at_ip(cs, temp_fixup)->uint = cs->section->insns_count;
    }
    if(node->else_body) {
        compile_node(cs, node->else_body, dest);
    } else {
        op_immediate(cs, cs->vm->lib.nil, dest);
    }
    for(size_t i = 0; i < node->case_count; i++) {
        insn_at_ip(cs, fixups[i])->uint = cs->section->insns_count;
    }
}

NODE(sl_node_while_t, while)
{
    next_last_frame_t nl;

    size_t begin = cs->section->insns_count;

    /* loop condition */
    compile_node(cs, node->expr, dest);

    /* emit code for !condition: */
    size_t fixup = op_jump_unless(cs, 0x0000CAFE, dest) + 1;

    /* push this loop on to the next/last fixup stack */
    nl.next_fixups = NULL;
    nl.last_fixups = NULL;
    nl.try_catch_blocks = 0;
    nl.prev = cs->next_last_frames;
    cs->next_last_frames = &nl;

    /* loop body */
    compile_node(cs, node->body, dest);

    /* place the right address in the next fixups */
    cs->next_last_frames = nl.prev;
    while(nl.next_fixups) {
        insn_at_ip(cs, nl.next_fixups->fixup)->uint = begin;
        nl.next_fixups = nl.next_fixups->next;
    }

    /* jump back to condition */
    op_jump(cs, begin);

    /* put the current IP into the JUMP_UNLESS fixup */
    insn_at_ip(cs, fixup)->uint = cs->section->insns_count;

    /* place the right address in the last fixups */
    while(nl.last_fixups) {
        insn_at_ip(cs, nl.last_fixups->fixup)->uint = cs->section->insns_count;
        nl.last_fixups = nl.last_fixups->next;
    }

    op_immediate(cs, cs->vm->lib.nil, dest);
}

NODE(sl_node_for_t, for)
{
    size_t expr_reg = reg_alloc(cs), enum_reg = reg_alloc(cs), has_looped_reg = reg_alloc(cs);
    size_t begin, end_jump_fixup, end_else_fixup;
    next_last_frame_t nl;

    compile_node(cs, node->expr, expr_reg);

    op_immediate(cs, cs->vm->lib._false, has_looped_reg);

    emit_send(cs, expr_reg, sl_intern(cs->vm, "enumerate"), 0, 0, enum_reg);

    begin = cs->section->insns_count;

    emit_send(cs, enum_reg, sl_intern(cs->vm, "next"), 0, 0, dest);

    end_jump_fixup = op_jump_unless(cs, 0x0000CAFE, dest) + 1;

    op_immediate(cs, cs->vm->lib._true, has_looped_reg);

    emit_send(cs, enum_reg, sl_intern(cs->vm, "current"), 0, 0, dest);

    emit_assignment(cs, node->lval, dest);

    nl.next_fixups = NULL;
    nl.last_fixups = NULL;
    nl.try_catch_blocks = 0;
    nl.prev = cs->next_last_frames;
    cs->next_last_frames = &nl;

    compile_node(cs, node->body, dest);

    op_jump(cs, begin);

    cs->next_last_frames = nl.prev;

    while(nl.next_fixups) {
        insn_at_ip(cs, nl.next_fixups->fixup)->uint = begin;
        nl.next_fixups = nl.next_fixups->next;
    }

    while(nl.last_fixups) {
        insn_at_ip(cs, nl.last_fixups->fixup)->uint = cs->section->insns_count;
        nl.last_fixups = nl.last_fixups->next;
    }

    insn_at_ip(cs, end_jump_fixup)->uint = cs->section->insns_count;

    end_else_fixup = op_jump_if(cs, 0x0000cafe, has_looped_reg) + 1;

    reg_free(cs, has_looped_reg);
    reg_free(cs, enum_reg);

    if(node->else_body) {
        compile_node(cs, node->else_body, dest);
    }

    insn_at_ip(cs, end_else_fixup)->uint = cs->section->insns_count;

    op_mov(cs, expr_reg, dest);

    reg_free(cs, expr_reg);
}

NODE(sl_node_send_t, send)
{
    size_t arg_base, i;
    /* compile the receiver into our 'dest' register */
    if(node->recv->type != SL_NODE_SELF) {
        compile_node(cs, node->recv, dest);
    }

    arg_base = node->arg_count ? reg_alloc_block(cs, node->arg_count) : 0;
    for(i = 0; i < node->arg_count; i++) {
        compile_node(cs, node->args[i], arg_base + i);
    }

    if(node->recv->type == SL_NODE_SELF) {
        emit_send_self(cs, node->id, arg_base, node->arg_count, dest);
    } else {
        emit_send(cs, dest, node->id, arg_base, node->arg_count, dest);
    }
    reg_free_block(cs, arg_base, node->arg_count);
}

NODE(sl_node_bind_method_t, bind_method)
{
    compile_node(cs, node->recv, dest);

    op_bind_method(cs, dest, node->id, dest);
}

NODE(sl_node_const_t, const)
{
    if(node->obj) {
        compile_node(cs, node->obj, dest);
        op_get_object_const(cs, dest, node->id, dest);
    } else {
        sl_vm_inline_constant_cache_t* icc = sl_alloc(cs->vm->arena, sizeof(*icc));
        op_get_const(cs, icc, node->id, dest);
    }
}

NODE(sl_node_binary_t, and)
{
    compile_node(cs, node->left, dest);

    size_t fixup = op_jump_unless(cs, 0x0000cafe, dest) + 1;

    compile_node(cs, node->right, dest);

    insn_at_ip(cs, fixup)->uint = cs->section->insns_count;
}

NODE(sl_node_binary_t, or)
{
    compile_node(cs, node->left, dest);

    size_t fixup = op_jump_if(cs, 0x000cafe, dest) + 1;

    compile_node(cs, node->right, dest);

    insn_at_ip(cs, fixup)->uint = cs->section->insns_count;
}

NODE(sl_node_unary_t, not)
{
    compile_node(cs, node->expr, dest);
    op_not(cs, dest, dest);
}

NODE(sl_node_assign_var_t, assign_var)
{
    size_t frame;
    sl_compile_state_t* xcs;
    size_t index;

    /* create variable before compiling rval so that constructs such as: f = f; work */
    xcs = cs;
    while(xcs) {
        if(sl_st_lookup(xcs->vars, (sl_st_data_t)node->lval->name, NULL)) {
            break;
        }
        xcs = xcs->parent;
    }
    if(!xcs) { /* variable does not exist yet */
        index = reg_alloc(cs);
        sl_st_insert(cs->vars, (sl_st_data_t)node->lval->name, (sl_st_data_t)index);
    }

    compile_node(cs, node->rval, dest);

    frame = 0;
    xcs = cs;
    while(xcs) {
        if(sl_st_lookup(xcs->vars, (sl_st_data_t)node->lval->name, (sl_st_data_t*)&index)) {
            if(frame == 0) {
                op_mov(cs, dest, index);
            } else {
                op_set_outer(cs, frame, index, dest);
                mark_upper_scopes_as_closure_unsafe(cs, frame);
            }
            return;
        }
        xcs = xcs->parent;
        frame++;
    }
}

NODE(sl_node_assign_ivar_t, assign_ivar)
{
    compile_node(cs, node->rval, dest);
    SLID id = sl_intern2(cs->vm, sl_make_ptr((sl_object_t*)node->lval->name));
    op_set_ivar(cs, id, dest);
}

NODE(sl_node_assign_cvar_t, assign_cvar)
{
    compile_node(cs, node->rval, dest);
    SLID id = sl_intern2(cs->vm, sl_make_ptr((sl_object_t*)node->lval->name));
    op_set_cvar(cs, id, dest);
}

static void
emit_send_compound_conditional_assign(sl_compile_state_t* cs, sl_node_send_t* lval, sl_node_base_t* rval, sl_vm_opcode_t opcode, size_t dest)
{
    size_t arg_base, receiver = reg_alloc(cs);
    size_t fixup, i;

    compile_node(cs, lval->recv, receiver);

    arg_base = reg_alloc_block(cs, lval->arg_count + 1);
    for(i = 0; i < lval->arg_count; i++) {
        compile_node(cs, lval->args[i], arg_base + i);
    }

    /* compile the lval */
    emit_send(cs, receiver, lval->id, arg_base, lval->arg_count, dest);

    if(opcode == SL_OP_JUMP_IF) {
        fixup = op_jump_if(cs, 0xdeadbeef, dest) + 1;
    } else {
        fixup = op_jump_unless(cs, 0xdeadbeef, dest) + 1;
    }

    /* compile the rval */
    compile_node(cs, rval, arg_base + lval->arg_count);

    /* call the = version of the method to assign the value back */

    SLID mid = sl_id_make_setter(cs->vm, lval->id);
    emit_send(cs, receiver, mid, arg_base, lval->arg_count + 1, dest);

    /* move the rval back to dest reg */
    op_mov(cs, arg_base + lval->arg_count, dest);

    insn_at_ip(cs, fixup)->uint = cs->section->insns_count;
}

NODE(sl_node_assign_send_t, assign_send)
{
    size_t arg_base, i, receiver;

    if(node->op_method && strcmp(node->op_method, "||") == 0) {
        emit_send_compound_conditional_assign(cs, node->lval, node->rval, SL_OP_JUMP_IF, dest);
        return;
    } else if(node->op_method && strcmp(node->op_method, "&&") == 0) {
        emit_send_compound_conditional_assign(cs, node->lval, node->rval, SL_OP_JUMP_UNLESS, dest);
        return;
    }

    receiver = reg_alloc(cs);

    compile_node(cs, node->lval->recv, receiver);

    arg_base = reg_alloc_block(cs, node->lval->arg_count + 1);
    for(i = 0; i < node->lval->arg_count; i++) {
        compile_node(cs, node->lval->args[i], arg_base + i);
    }

    if(node->op_method == NULL) {
        compile_node(cs, node->rval, arg_base + node->lval->arg_count);
    } else {
        /* compile the lval */
        emit_send(cs, receiver, node->lval->id, arg_base, node->lval->arg_count, dest);

        /* compile the rval */
        compile_node(cs, node->rval, arg_base + node->lval->arg_count);

        emit_send(cs, dest, sl_intern(cs->vm, node->op_method), arg_base + node->lval->arg_count, 1, arg_base + node->lval->arg_count);
    }

    /* call the = version of the method to assign the value back */
    SLID mid = sl_id_make_setter(cs->vm, node->lval->id);
    emit_send(cs, receiver, mid, arg_base, node->lval->arg_count + 1, dest);

    op_mov(cs, arg_base + node->lval->arg_count, dest);

    reg_free(cs, receiver);
    reg_free_block(cs, arg_base, node->lval->arg_count + 1);
}

NODE(sl_node_assign_const_t, assign_const)
{
    size_t reg;
    if(node->lval->obj) {
        /* SL_OP_SET_OBJECT_CONST */
        reg = reg_alloc(cs);
        compile_node(cs, node->lval->obj, reg);
        compile_node(cs, node->rval, dest);
        op_set_object_const(cs, reg, node->lval->id, dest);
        reg_free(cs, reg);
    } else {
        /* SL_OP_SET_CONST */
        compile_node(cs, node->rval, dest);
        op_set_const(cs, node->lval->id, dest);
    }
}

NODE(sl_node_assign_array_t, assign_array)
{
    size_t dump_regs, i;
    dump_regs = reg_alloc_block(cs, node->lval->node_count);

    compile_node(cs, node->rval, dest);

    op_array_dump(cs, dest, node->lval->node_count, dump_regs);

    for(i = 0; i < node->lval->node_count; i++) {
        emit_assignment(cs, node->lval->nodes[i], dump_regs + i);
    }

    reg_free_block(cs, dump_regs, node->lval->node_count);
}

NODE(sl_node_mutate_t, prefix_mutate)
{
    if(node->lval->type == SL_NODE_SEND) {
        sl_node_send_t* send = (sl_node_send_t*)node->lval;
        size_t recv = reg_alloc(cs);
        compile_node(cs, send->recv, recv);

        size_t args_regs = reg_alloc_block(cs, send->arg_count + 1);
        for(size_t i = 0; i < send->arg_count; i++) {
            compile_node(cs, send->args[i], args_regs + i);
        }
        emit_send(cs, recv, send->id, args_regs, send->arg_count, dest);

        emit_send(cs, dest, sl_intern(cs->vm, node->op_method), 0, 0, args_regs + send->arg_count);

        SLID assign_id = sl_id_make_setter(cs->vm, send->id);
        emit_send(cs, recv, assign_id, args_regs, send->arg_count + 1, recv);

        reg_free(cs, recv);
        reg_free_block(cs, args_regs, send->arg_count + 1);

        op_mov(cs, args_regs + send->arg_count, dest);
    } else {
        compile_node(cs, node->lval, dest);
        emit_send(cs, dest, sl_intern(cs->vm, node->op_method), 0, 0, dest);
        emit_assignment(cs, node->lval, dest);
    }
}

NODE(sl_node_mutate_t, postfix_mutate)
{
    if(node->lval->type == SL_NODE_SEND) {
        sl_node_send_t* send = (sl_node_send_t*)node->lval;
        size_t recv = reg_alloc(cs);
        compile_node(cs, send->recv, recv);

        size_t args_regs = reg_alloc_block(cs, send->arg_count + 1);
        for(size_t i = 0; i < send->arg_count; i++) {
            compile_node(cs, send->args[i], args_regs + i);
        }
        emit_send(cs, recv, send->id, args_regs, send->arg_count, dest);

        emit_send(cs, dest, sl_intern(cs->vm, node->op_method), 0, 0, args_regs + send->arg_count);

        SLID assign_id = sl_id_make_setter(cs->vm, send->id);
        emit_send(cs, recv, assign_id, args_regs, send->arg_count + 1, recv);

        reg_free(cs, recv);
        reg_free_block(cs, args_regs, send->arg_count + 1);
    } else {
        size_t temp_reg = reg_alloc(cs);
        compile_node(cs, node->lval, dest);
        emit_send(cs, dest, sl_intern(cs->vm, node->op_method), 0, 0, temp_reg);
        emit_assignment(cs, node->lval, temp_reg);
        reg_free(cs, temp_reg);
    }
}

NODE(sl_node_array_t, array)
{
    size_t i, reg_base = reg_alloc_block(cs, node->node_count);
    for(i = 0; i < node->node_count; i++) {
        compile_node(cs, node->nodes[i], reg_base + i);
    }
    op_array(cs, node->node_count, reg_base, dest);
    reg_free_block(cs, reg_base, node->node_count);
}

NODE(sl_node_dict_t, dict)
{
    size_t i, reg_base = reg_alloc_block(cs, node->node_count * 2);
    for(i = 0; i < node->node_count; i++) {
        compile_node(cs, node->keys[i], reg_base + i * 2);
        compile_node(cs, node->vals[i], reg_base + i * 2 + 1);
    }
    op_dict(cs, node->node_count, reg_base, dest);
    reg_free_block(cs, reg_base, node->node_count * 2);
}

NODE(sl_node_unary_t, return)
{
    compile_node(cs, node->expr, dest);
    op_return(cs, dest);
}

NODE(sl_node_range_t, range)
{
    size_t left = dest, right = reg_alloc(cs);
    compile_node(cs, node->left, left);
    compile_node(cs, node->right, right);
    if(node->exclusive) {
        op_range_ex(cs, left, right, dest);
    } else {
        op_range_in(cs, left, right, dest);
    }
    reg_free(cs, right);
}

NODE(sl_node_base_t, next)
{
    fixup_t* fixup = sl_alloc(cs->vm->arena, sizeof(fixup_t));
    fixup->next = cs->next_last_frames->next_fixups;
    cs->next_last_frames->next_fixups = fixup;
    for(size_t i = 0; i < cs->next_last_frames->try_catch_blocks; i++) {
        op_end_try(cs);
    }
    fixup->fixup = op_jump(cs, 0x0000cafe) + 1;

    (void)node;
    (void)dest;
}

NODE(sl_node_base_t, last)
{
    fixup_t* fixup = sl_alloc(cs->vm->arena, sizeof(fixup_t));
    fixup->next = cs->next_last_frames->last_fixups;
    cs->next_last_frames->last_fixups = fixup;
    for(size_t i = 0; i < cs->next_last_frames->try_catch_blocks; i++) {
        op_end_try(cs);
    }
    fixup->fixup = op_jump(cs, 0x0000cafe) + 1;

    (void)node;
    (void)dest;
}

NODE(sl_node_unary_t, throw)
{
    compile_node(cs, node->expr, dest);
    op_throw(cs, dest);
}

NODE(sl_node_base_t, yada_yada)
{
    op_yada_yada(cs);
    (void)dest;
    (void)node;
}

NODE(sl_node_const_t, use)
{
    if(node->obj) {
        compile_node(cs, node->obj, dest);
        op_use(cs, dest, node->id, dest);
    } else {
        op_use_top_level(cs, node->id, dest);
    }
}

NODE(sl_node__register_t, _register)
{
    op_mov(cs, node->reg, dest);
}

#define COMPILE(type, caps, name) case SL_NODE_##caps: compile_##name(cs, (type*)node, dest); return;

static void
emit_line_trace(sl_compile_state_t* cs, sl_node_base_t* node)
{
    if(node->line != cs->last_line && node->line != 0) {
        if(cs->emitted_line_trace) {
            insn_at_ip(cs, cs->section->insns_count - 1)->uint = node->line;
        } else {
            cs->last_line = node->line;
            op_line_trace(cs, node->line);
        }
    }
}

static void
compile_node(sl_compile_state_t* cs, sl_node_base_t* node, size_t dest)
{
    emit_line_trace(cs, node);
    switch(node->type) {
        COMPILE(sl_node_seq_t,           SEQ,            seq);
        COMPILE(sl_node_raw_t,           RAW,            raw);
        COMPILE(sl_node_echo_t,          ECHO,           echo);
        COMPILE(sl_node_echo_t,          ECHO_RAW,       echo_raw);
        COMPILE(sl_node_var_t,           VAR,            var);
        COMPILE(sl_node_var_t,           IVAR,           ivar);
        COMPILE(sl_node_var_t,           CVAR,           cvar);
        COMPILE(sl_node_immediate_t,     IMMEDIATE,      immediate);
        COMPILE(sl_node_interp_string_t, INTERP_STRING,  interp_string);
        COMPILE(sl_node_base_t,          SELF,           self);
        COMPILE(sl_node_class_t,         CLASS,          class);
        COMPILE(sl_node_def_t,           DEF,            def);
        COMPILE(sl_node_lambda_t,        LAMBDA,         lambda);
        COMPILE(sl_node_try_t,           TRY,            try);
        COMPILE(sl_node_if_t,            IF,             if);
        COMPILE(sl_node_switch_t,        SWITCH,         switch);
        COMPILE(sl_node_while_t,         WHILE,          while);
        COMPILE(sl_node_for_t,           FOR,            for);
        COMPILE(sl_node_send_t,          SEND,           send);
        COMPILE(sl_node_bind_method_t,   BIND_METHOD,    bind_method);
        COMPILE(sl_node_const_t,         CONST,          const);
        COMPILE(sl_node_binary_t,        AND,            and);
        COMPILE(sl_node_binary_t,        OR,             or);
        COMPILE(sl_node_unary_t,         NOT,            not);
        COMPILE(sl_node_assign_var_t,    ASSIGN_VAR,     assign_var);
        COMPILE(sl_node_assign_ivar_t,   ASSIGN_IVAR,    assign_ivar);
        COMPILE(sl_node_assign_cvar_t,   ASSIGN_CVAR,    assign_cvar);
        COMPILE(sl_node_assign_send_t,   ASSIGN_SEND,    assign_send);
        COMPILE(sl_node_assign_const_t,  ASSIGN_CONST,   assign_const);
        COMPILE(sl_node_assign_array_t,  ASSIGN_ARRAY,   assign_array);
        COMPILE(sl_node_mutate_t,        PREFIX_MUTATE,  prefix_mutate);
        COMPILE(sl_node_mutate_t,        POSTFIX_MUTATE, postfix_mutate);
        COMPILE(sl_node_array_t,         ARRAY,          array);
        COMPILE(sl_node_dict_t,          DICT,           dict);
        COMPILE(sl_node_unary_t,         RETURN,         return);
        COMPILE(sl_node_range_t,         RANGE,          range);
        COMPILE(sl_node_base_t,          NEXT,           next);
        COMPILE(sl_node_base_t,          LAST,           last);
        COMPILE(sl_node_unary_t,         THROW,          throw);
        COMPILE(sl_node_base_t,          YADA_YADA,      yada_yada);
        COMPILE(sl_node_const_t,         USE,            use);
        COMPILE(sl_node__register_t,     _REGISTER,      _register);
    }
    sl_throw_message(cs->vm, "Unknown node type in compile_node");
}

sl_vm_section_t*
sl_compile(sl_vm_t* vm, sl_node_base_t* ast, uint8_t* filename)
{
    sl_compile_state_t cs;
    init_compile_state(&cs, vm, NULL, 1);
    cs.section->filename = filename;
    cs.section->name = sl_intern(vm, "<main>");

    compile_node(&cs, ast, 0);

    op_return(&cs, 0);

    return cs.section;
}
