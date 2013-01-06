#include <slash/compile.h>
#include <slash/string.h>
#include <slash/method.h>
#include <string.h>

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
    st_table_t* vars;
    struct sl_compile_state* parent;
    int last_line;
    uint8_t* registers;
    sl_vm_section_t* section;
    next_last_frame_t* next_last_frames;
}
sl_compile_state_t;

static void
init_compile_state(sl_compile_state_t* cs, sl_vm_t* vm, sl_compile_state_t* parent, size_t init_registers)
{
    size_t i;
    cs->vm = vm;
    cs->vars = st_init_table(vm->arena, &sl_string_hash_type);
    cs->parent = parent;
    cs->last_line = 0;
    cs->section = sl_alloc(vm->arena, sizeof(sl_vm_section_t));
    if(parent) {
        cs->section->filename = parent->section->filename;
    }
    cs->section->max_registers = init_registers;
    cs->section->req_registers = 0;
    cs->section->arg_registers = 0;
    cs->section->insns_cap = 4;
    cs->section->insns_count = 0;
    cs->section->insns = sl_alloc(vm->arena, sizeof(sl_vm_insn_t) * cs->section->insns_cap);
    cs->section->can_stack_alloc_frame = 1;
    cs->section->opt_skip = NULL;
    cs->registers = sl_alloc(vm->arena, cs->section->max_registers);
    for(i = 0; i < init_registers; i++) {
        cs->registers[i] = 1;
    }
    cs->next_last_frames = NULL;
}

static size_t
reg_alloc(sl_compile_state_t* cs)
{
    size_t i;
    for(i = 0; i < cs->section->max_registers; i++) {
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
reg_alloc_block(sl_compile_state_t* cs, size_t count)
{
    if(count == 0) {
        return 0;
    }
    
    size_t i, j;
    for(i = 0; i + count < cs->section->max_registers + 1; i++) {
        if(cs->registers[i]) {
            continue_outer: continue;
        }
        for(j = 0; j < count; j++) {
            if(cs->registers[i + j]) {
                i += j;
                goto continue_outer;
            }
        }
        for(j = 0; j < count; j++) {
            cs->registers[i + j] = 1;
        }
        return i;
    }
    cs->section->max_registers += count;
    cs->registers = sl_realloc(cs->vm->arena, cs->registers, cs->section->max_registers);
    i = cs->section->max_registers - count;
    for(j = 0; j < count; j++) {
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
reg_free_block(sl_compile_state_t* cs, size_t reg, size_t count)
{
    size_t i;
    for(i = 0; i < count; i++) {
        reg_free(cs, reg + i);
    }
}

static size_t
emit(sl_compile_state_t* cs, sl_vm_insn_t insn)
{
    if(cs->section->insns_count == cs->section->insns_cap) {
        cs->section->insns_cap *= 2;
        cs->section->insns = sl_realloc(cs->vm->arena, cs->section->insns, sizeof(sl_vm_insn_t) * cs->section->insns_cap);
    }
    cs->section->insns[cs->section->insns_count++] = insn;
    return cs->section->insns_count - 1;
}

static void
emit_immediate(sl_compile_state_t* cs, SLVAL immediate, size_t dest)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_IMMEDIATE;
    emit(cs, insn);
    insn.imm = immediate;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

static void
emit_send(sl_compile_state_t* cs, size_t recv, SLID id, size_t arg_base, size_t arg_size, size_t return_reg)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_SEND;
    emit(cs, insn);
    insn.uint = recv;
    emit(cs, insn);
    insn.id = id;
    emit(cs, insn);
    insn.uint = arg_base;
    emit(cs, insn);
    insn.uint = arg_size;
    emit(cs, insn);
    insn.uint = return_reg;
    emit(cs, insn);
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
            send.id = sl_intern2(cs->vm,
                sl_string_concat(cs->vm, sl_id_to_string(cs->vm, send.id), sl_make_cstring(cs->vm, "=")));
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
        emit_immediate(cs, cs->vm->lib.nil, dest);
    }
}

NODE(sl_node_raw_t, raw)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_RAW;
    emit(cs, insn);
    insn.imm = node->string;
    emit(cs, insn);
    (void)dest;
}

NODE(sl_node_echo_t, echo)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->expr, dest);
    insn.opcode = SL_OP_ECHO;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_echo_t, echo_raw)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->expr, dest);
    insn.opcode = SL_OP_ECHO_RAW;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
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
    sl_vm_insn_t insn;
    size_t frame;
    sl_compile_state_t* xcs = cs;
    size_t index = 0xCAFE;
    SLVAL err;
    frame = 0;
    while(xcs) {
        if(st_lookup(xcs->vars, (st_data_t)node->name, (st_data_t*)&index)) {
            if(frame == 0) {
                insn.opcode = SL_OP_MOV;
                emit(cs, insn);
            } else {
                insn.opcode = SL_OP_GET_OUTER;
                emit(cs, insn);
                insn.uint = frame;
                emit(cs, insn);
                mark_upper_scopes_as_closure_unsafe(cs, frame);
            }
            insn.uint = index;
            emit(cs, insn);
            insn.uint = dest;
            emit(cs, insn);
            return;
        }
        xcs = xcs->parent;
        frame++;
    }
    err = sl_make_cstring(cs->vm, "Undefined variable '");
    err = sl_string_concat(cs->vm, err, sl_make_ptr((sl_object_t*)node->name));
    err = sl_string_concat(cs->vm, err, sl_make_cstring(cs->vm, "' "));
    err = sl_make_error2(cs->vm, cs->vm->lib.NameError, err);
    sl_error_add_frame(cs->vm, err, sl_make_cstring(cs->vm, "<compiler>"), sl_make_cstring(cs->vm, (char*)cs->section->filename), sl_make_int(cs->vm, node->base.line));
    sl_throw(cs->vm, err);
}

NODE(sl_node_var_t, ivar)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_GET_IVAR;
    emit(cs, insn);
    insn.str = node->name;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_var_t, cvar)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_GET_CVAR;
    emit(cs, insn);
    insn.str = node->name;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}


NODE(sl_node_immediate_t, immediate)
{
    emit_immediate(cs, node->value, dest);
}

NODE(sl_node_base_t, self)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_SELF;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    (void)node;
}

NODE(sl_node_def_t, def)
{
    sl_vm_insn_t insn;
    sl_compile_state_t sub_cs;
    size_t i, on_reg;
    init_compile_state(&sub_cs, cs->vm, cs, node->req_arg_count + node->opt_arg_count + 1);
    for(i = 0; i < node->req_arg_count; i++) {
        st_insert(sub_cs.vars, (st_data_t)node->req_args[i], (st_data_t)(i + 1));
    }
    for(i = 0; i < node->opt_arg_count; i++) {
        st_insert(sub_cs.vars, (st_data_t)node->opt_args[i].name, (st_data_t)(node->req_arg_count + i + 1));
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
    insn.opcode = SL_OP_RETURN;
    emit(&sub_cs, insn);
    insn.uint = 0;
    emit(&sub_cs, insn);
    
    if(node->on) {
        on_reg = reg_alloc(cs);
        compile_node(cs, node->on, on_reg);
        insn.opcode = SL_OP_DEFINE_ON;
        emit(cs, insn);
        insn.uint = on_reg;
        emit(cs, insn);
        reg_free(cs, on_reg);
    } else {
        insn.opcode = SL_OP_DEFINE;
        emit(cs, insn);
    }
    insn.id = node->name;
    emit(cs, insn);
    insn.section = sub_cs.section;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_lambda_t, lambda)
{
    sl_vm_insn_t insn;
    sl_compile_state_t sub_cs;
    size_t i;
    init_compile_state(&sub_cs, cs->vm, cs, node->arg_count + 1);
    for(i = 0; i < node->arg_count; i++) {
        st_insert(sub_cs.vars, (st_data_t)node->args[i], (st_data_t)(i + 1));
    }
    sub_cs.section->req_registers = node->arg_count;
    sub_cs.section->arg_registers = node->arg_count;
    sub_cs.section->name = sl_intern(cs->vm, "<lambda>");
    compile_node(&sub_cs, node->body, 0);
    insn.opcode = SL_OP_RETURN;
    emit(&sub_cs, insn);
    insn.uint = 0;
    emit(&sub_cs, insn);
    
    insn.opcode = SL_OP_LAMBDA;
    emit(cs, insn);
    insn.section = sub_cs.section;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_try_t, try)
{
    sl_vm_insn_t insn;
    size_t catch_fixup, after_fixup;
    
    insn.opcode = SL_OP_TRY;
    emit(cs, insn);
    insn.uint = 0xdeadbeef;
    catch_fixup = emit(cs, insn);
    
    if(cs->next_last_frames) {
        cs->next_last_frames->try_catch_blocks++;
    }
    
    compile_node(cs, node->body, dest);

    if(cs->next_last_frames) {
        cs->next_last_frames->try_catch_blocks--;
    }

    insn.opcode = SL_OP_END_TRY;
    emit(cs, insn);
    
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = 0xdeadbeef;
    after_fixup = emit(cs, insn);
    
    cs->section->insns[catch_fixup].uint = cs->section->insns_count;
    
    insn.opcode = SL_OP_CATCH;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    insn.opcode = SL_OP_END_TRY;
    emit(cs, insn);
    
    emit_assignment(cs, node->lval, dest);
    compile_node(cs, node->catch_body, dest);
    
    cs->section->insns[after_fixup].uint = cs->section->insns_count;
}

NODE(sl_node_class_t, class)
{
    sl_vm_insn_t insn;
    sl_compile_state_t sub_cs;
    
    if(node->extends) {
        compile_node(cs, node->extends, dest);
    } else {
        emit_immediate(cs, cs->vm->lib.Object, dest);
    }
    
    init_compile_state(&sub_cs, cs->vm, cs, 1);
    sub_cs.section->name = sl_intern2(cs->vm,
        sl_string_concat(cs->vm, sl_make_cstring(cs->vm, "class "), sl_id_to_string(cs->vm, node->name)));
    compile_node(&sub_cs, node->body, 0);
    insn.opcode = SL_OP_RETURN;
    emit(&sub_cs, insn);
    insn.uint = 0;
    emit(&sub_cs, insn);
    
    insn.opcode = SL_OP_CLASS;
    emit(cs, insn);
    insn.id = node->name;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    insn.section = sub_cs.section;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

/* @TODO: class, def, lambda, try */

NODE(sl_node_if_t, if)
{
    sl_vm_insn_t insn;
    size_t fixup;
    
    /* emit a jump over the true branch, keeping a pointer to fixup later */
    compile_node(cs, node->condition, dest);
    insn.opcode = SL_OP_JUMP_UNLESS;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    /* true branch */
    compile_node(cs, node->body, dest);
    
    /*  emit a jump over the else body, and compensate for the jump
        by adding two bytes to the fixup's operand */
    cs->section->insns[fixup].uint = cs->section->insns_count + 2 /* JUMP <end> */;
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup = emit(cs, insn);
    
    if(node->else_body) {
        compile_node(cs, node->else_body, dest);
    } else {
        emit_immediate(cs, cs->vm->lib.nil, dest);
    }
    
    cs->section->insns[fixup].uint = cs->section->insns_count;
}

NODE(sl_node_while_t, while)
{
    sl_vm_insn_t insn;
    size_t fixup, begin;
    next_last_frame_t nl;
    
    begin = cs->section->insns_count;
    
    /* loop condition */
    compile_node(cs, node->expr, dest);
    
    /* emit code for !condition: */
    insn.opcode = SL_OP_JUMP_UNLESS;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
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
        cs->section->insns[nl.next_fixups->fixup].uint = begin;
        nl.next_fixups = nl.next_fixups->next;
    }
    
    /* jump back to condition */
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = begin;
    emit(cs, insn);
    
    /* put the current IP into the JUMP_UNLESS fixup */
    cs->section->insns[fixup].uint = cs->section->insns_count;
    
    /* place the right address in the last fixups */
    while(nl.last_fixups) {
        cs->section->insns[nl.last_fixups->fixup].uint = cs->section->insns_count;
        nl.last_fixups = nl.last_fixups->next;
    }
    
    emit_immediate(cs, cs->vm->lib.nil, dest);
}

NODE(sl_node_for_t, for)
{
    sl_vm_insn_t insn;
    size_t expr_reg = reg_alloc(cs), enum_reg = reg_alloc(cs), has_looped_reg = reg_alloc(cs);
    size_t begin, end_jump_fixup, end_else_fixup;
    next_last_frame_t nl;
    
    compile_node(cs, node->expr, expr_reg);
    
    insn.opcode = SL_OP_IMMEDIATE;
    emit(cs, insn);
    insn.imm = cs->vm->lib._false;
    emit(cs, insn);
    insn.uint = has_looped_reg;
    emit(cs, insn);
    
    emit_send(cs, expr_reg, sl_intern(cs->vm, "enumerate"), 0, 0, enum_reg);
    
    begin = cs->section->insns_count;
    
    emit_send(cs, enum_reg, sl_intern(cs->vm, "next"), 0, 0, dest);
    
    insn.opcode = SL_OP_JUMP_UNLESS;
    emit(cs, insn);
    insn.opcode = 0x0000cafe;
    end_jump_fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    insn.opcode = SL_OP_IMMEDIATE;
    emit(cs, insn);
    insn.imm = cs->vm->lib._true;
    emit(cs, insn);
    insn.uint = has_looped_reg;
    emit(cs, insn);
    
    emit_send(cs, enum_reg, sl_intern(cs->vm, "current"), 0, 0, dest);
    
    emit_assignment(cs, node->lval, dest);
    
    nl.next_fixups = NULL;
    nl.last_fixups = NULL;
    nl.try_catch_blocks = 0;
    nl.prev = cs->next_last_frames;
    cs->next_last_frames = &nl;
    
    compile_node(cs, node->body, dest);
    
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = begin;
    emit(cs, insn);
    
    cs->next_last_frames = nl.prev;
    
    while(nl.next_fixups) {
        cs->section->insns[nl.next_fixups->fixup].uint = begin;
        nl.next_fixups = nl.next_fixups->next;
    }
    
    while(nl.last_fixups) {
        cs->section->insns[nl.last_fixups->fixup].uint = cs->section->insns_count;
        nl.last_fixups = nl.last_fixups->next;
    }
    
    cs->section->insns[end_jump_fixup].uint = cs->section->insns_count;
    
    insn.opcode = SL_OP_JUMP_IF;
    emit(cs, insn);
    insn.uint = 0x0000cafe;
    end_else_fixup = emit(cs, insn);
    insn.uint = has_looped_reg;
    emit(cs, insn);
    
    reg_free(cs, has_looped_reg);
    reg_free(cs, enum_reg);
    
    if(node->else_body) {
        compile_node(cs, node->else_body, dest);
    }
    
    cs->section->insns[end_else_fixup].uint = cs->section->insns_count;
    
    insn.opcode = SL_OP_MOV;
    emit(cs, insn);
    insn.uint = expr_reg;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    reg_free(cs, expr_reg);
}

NODE(sl_node_send_t, send)
{
    size_t arg_base, i;
    /* compile the receiver into our 'dest' register */
    compile_node(cs, node->recv, dest);
    arg_base = node->arg_count ? reg_alloc_block(cs, node->arg_count) : 0;
    for(i = 0; i < node->arg_count; i++) {
        compile_node(cs, node->args[i], arg_base + i);
    }
    
    emit_send(cs, dest, node->id, arg_base, node->arg_count, dest);
    reg_free_block(cs, arg_base, node->arg_count);
}

NODE(sl_node_bind_method_t, bind_method)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->recv, dest);
    
    insn.opcode = SL_OP_BIND_METHOD;
    emit(cs, insn);
    insn.uint = dest; /* recv */
    emit(cs, insn);
    insn.id = node->id; /* id */
    emit(cs, insn);
    insn.uint = dest; /* dest */
    emit(cs, insn);
}

NODE(sl_node_const_t, const)
{
    sl_vm_insn_t insn;
    if(node->obj) {
        compile_node(cs, node->obj, dest);
        insn.opcode = SL_OP_GET_OBJECT_CONST;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
        insn.id = node->id;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
    } else {
        insn.opcode = SL_OP_GET_CONST;
        emit(cs, insn);
        insn.ic = sl_alloc(cs->vm->arena, sizeof(sl_vm_inline_cache_t));
        emit(cs, insn);
        insn.id = node->id;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
    }
}

NODE(sl_node_binary_t, and)
{
    sl_vm_insn_t insn;
    size_t fixup;
    
    compile_node(cs, node->left, dest);
    
    insn.opcode = SL_OP_JUMP_UNLESS;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    compile_node(cs, node->right, dest);
    
    cs->section->insns[fixup].uint = cs->section->insns_count;
}

NODE(sl_node_binary_t, or)
{
    sl_vm_insn_t insn;
    size_t fixup;
    
    compile_node(cs, node->left, dest);
    
    insn.opcode = SL_OP_JUMP_IF;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    compile_node(cs, node->right, dest);
    
    cs->section->insns[fixup].uint = cs->section->insns_count;
}

NODE(sl_node_unary_t, not)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->expr, dest);
    insn.opcode = SL_OP_NOT;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_assign_var_t, assign_var)
{
    sl_vm_insn_t insn;
    size_t frame;
    sl_compile_state_t* xcs;
    size_t index;
    
    /* create variable before compiling rval so that constructs such as: f = f; work */
    xcs = cs;
    while(xcs) {
        if(st_lookup(xcs->vars, (st_data_t)node->lval->name, NULL)) {
            break;
        }
        xcs = xcs->parent;
    }
    if(!xcs) { /* variable does not exist yet */
        index = reg_alloc(cs);
        st_insert(cs->vars, (st_data_t)node->lval->name, (st_data_t)index);
    }
    
    compile_node(cs, node->rval, dest);
    
    frame = 0;
    xcs = cs;
    while(xcs) {
        if(st_lookup(xcs->vars, (st_data_t)node->lval->name, (st_data_t*)&index)) {
            if(frame == 0) {
                insn.opcode = SL_OP_MOV;
                emit(cs, insn);
                insn.uint = dest;
                emit(cs, insn);
                insn.uint = index;
                emit(cs, insn);
            } else {
                insn.opcode = SL_OP_SET_OUTER;
                emit(cs, insn);
                insn.uint = frame;
                emit(cs, insn);
                insn.uint = index;
                emit(cs, insn);
                insn.uint = dest;
                emit(cs, insn);
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
    sl_vm_insn_t insn;
    compile_node(cs, node->rval, dest);
    insn.opcode = SL_OP_SET_IVAR;
    emit(cs, insn);
    insn.str = node->lval->name;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_assign_cvar_t, assign_cvar)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->rval, dest);
    insn.opcode = SL_OP_SET_CVAR;
    emit(cs, insn);
    insn.str = node->lval->name;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

static void
emit_send_compound_conditional_assign(sl_compile_state_t* cs, sl_node_send_t* lval, sl_node_base_t* rval, sl_vm_opcode_t opcode, size_t dest)
{
    sl_vm_insn_t insn;
    size_t arg_base, receiver = reg_alloc(cs);
    size_t fixup, i;
    
    compile_node(cs, lval->recv, receiver);
    
    arg_base = reg_alloc_block(cs, lval->arg_count + 1);
    for(i = 0; i < lval->arg_count; i++) {
        compile_node(cs, lval->args[i], arg_base + i);
    }
    
    /* compile the lval */
    emit_send(cs, receiver, lval->id, arg_base, lval->arg_count, dest);

    insn.opcode = opcode; /* SL_OP_JUMP_{IF, UNLESS} */
    emit(cs, insn);
    insn.uint = 0xdeafbeef;
    fixup = emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    /* compile the rval */
    compile_node(cs, rval, arg_base + lval->arg_count);

    /* call the = version of the method to assign the value back */
    SLVAL mid = sl_string_concat(cs->vm, sl_id_to_string(cs->vm, lval->id), sl_make_cstring(cs->vm, "="));
    emit_send(cs, receiver, sl_intern2(cs->vm, mid), arg_base, lval->arg_count + 1, dest);
    
    /* move the rval back to dest reg */
    insn.opcode = SL_OP_MOV;
    emit(cs, insn);
    insn.uint = arg_base + lval->arg_count;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    cs->section->insns[fixup].uint = cs->section->insns_count;
}

NODE(sl_node_assign_send_t, assign_send)
{
    sl_vm_insn_t insn;
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
    SLVAL mid = sl_string_concat(cs->vm, sl_id_to_string(cs->vm, node->lval->id), sl_make_cstring(cs->vm, "="));
    emit_send(cs, receiver, sl_intern2(cs->vm, mid), arg_base, node->lval->arg_count + 1, dest);
    
    insn.opcode = SL_OP_MOV;
    emit(cs, insn);
    insn.uint = arg_base + node->lval->arg_count;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    
    reg_free(cs, receiver);
    reg_free_block(cs, arg_base, node->lval->arg_count + 1);
}

NODE(sl_node_assign_const_t, assign_const)
{
    sl_vm_insn_t insn;
    size_t reg;
    if(node->lval->obj) {
        /* SL_OP_SET_OBJECT_CONST */
        reg = reg_alloc(cs);
        compile_node(cs, node->lval->obj, reg);
        compile_node(cs, node->rval, dest);
        insn.opcode = SL_OP_SET_OBJECT_CONST;
        emit(cs, insn);
        insn.uint = reg;
        emit(cs, insn);
        insn.id = node->lval->id;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
        reg_free(cs, reg);
    } else {
        /* SL_OP_SET_CONST */
        compile_node(cs, node->rval, dest);
        insn.opcode = SL_OP_SET_CONST;
        emit(cs, insn);
        insn.id = node->lval->id;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
    }
}

NODE(sl_node_assign_array_t, assign_array)
{
    sl_vm_insn_t insn;
    size_t dump_regs, i;
    dump_regs = reg_alloc_block(cs, node->lval->node_count);
    
    compile_node(cs, node->rval, dest);
    
    insn.opcode = SL_OP_ARRAY_DUMP;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    insn.uint = node->lval->node_count;
    emit(cs, insn);
    insn.uint = dump_regs;
    emit(cs, insn);
    
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
        
        SLVAL assign_id = sl_string_concat(cs->vm, sl_id_to_string(cs->vm, send->id), sl_make_cstring(cs->vm, "="));
        emit_send(cs, recv, sl_intern2(cs->vm, assign_id), args_regs, send->arg_count + 1, recv);
        
        reg_free(cs, recv);
        reg_free_block(cs, args_regs, send->arg_count + 1);
        
        sl_vm_insn_t insn;
        insn.opcode = SL_OP_MOV;
        emit(cs, insn);
        insn.uint = args_regs + send->arg_count;
        emit(cs, insn);
        insn.uint = dest;
        emit(cs, insn);
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
        
        SLVAL assign_id = sl_string_concat(cs->vm, sl_id_to_string(cs->vm, send->id), sl_make_cstring(cs->vm, "="));
        emit_send(cs, recv, sl_intern2(cs->vm, assign_id), args_regs, send->arg_count + 1, recv);
        
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
    sl_vm_insn_t insn;
    size_t i, reg_base = reg_alloc_block(cs, node->node_count);
    for(i = 0; i < node->node_count; i++) {
        compile_node(cs, node->nodes[i], reg_base + i);
    }
    insn.opcode = SL_OP_ARRAY;
    emit(cs, insn);
    insn.uint = node->node_count;
    emit(cs, insn);
    insn.uint = reg_base;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    reg_free_block(cs, reg_base, node->node_count);
}

NODE(sl_node_dict_t, dict)
{
    sl_vm_insn_t insn;
    size_t i, reg_base = reg_alloc_block(cs, node->node_count * 2);
    for(i = 0; i < node->node_count; i++) {
        compile_node(cs, node->keys[i], reg_base + i * 2);
        compile_node(cs, node->vals[i], reg_base + i * 2 + 1);
    }
    insn.opcode = SL_OP_DICT;
    emit(cs, insn);
    insn.uint = node->node_count;
    emit(cs, insn);
    insn.uint = reg_base;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    reg_free_block(cs, reg_base, node->node_count * 2);
}

NODE(sl_node_unary_t, return)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->expr, dest);
    insn.opcode = SL_OP_RETURN;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_range_t, range)
{
    sl_vm_insn_t insn;
    size_t left = dest, right = reg_alloc(cs);
    compile_node(cs, node->left, left);
    compile_node(cs, node->right, right);
    if(node->exclusive) {
        insn.opcode = SL_OP_RANGE_EX;
    } else {
        insn.opcode = SL_OP_RANGE;
    }
    emit(cs, insn);
    insn.uint = left;
    emit(cs, insn);
    insn.uint = right;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
    reg_free(cs, right);
}

NODE(sl_node_base_t, next)
{
    sl_vm_insn_t insn;
    fixup_t* fixup = sl_alloc(cs->vm->arena, sizeof(fixup_t));
    fixup->next = cs->next_last_frames->next_fixups;
    cs->next_last_frames->next_fixups = fixup;
    for(size_t i = 0; i < cs->next_last_frames->try_catch_blocks; i++) {
        insn.opcode = SL_OP_END_TRY;
        emit(cs, insn);
    }
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup->fixup = emit(cs, insn);
    
    (void)node;
    (void)dest;
}

NODE(sl_node_base_t, last)
{
    sl_vm_insn_t insn;
    fixup_t* fixup = sl_alloc(cs->vm->arena, sizeof(fixup_t));
    fixup->next = cs->next_last_frames->last_fixups;
    cs->next_last_frames->last_fixups = fixup;
    for(size_t i = 0; i < cs->next_last_frames->try_catch_blocks; i++) {
        insn.opcode = SL_OP_END_TRY;
        emit(cs, insn);
    }
    insn.opcode = SL_OP_JUMP;
    emit(cs, insn);
    insn.uint = 0x0000CAFE;
    fixup->fixup = emit(cs, insn);
    
    (void)node;
    (void)dest;
}

NODE(sl_node_unary_t, throw)
{
    sl_vm_insn_t insn;
    compile_node(cs, node->expr, dest);
    insn.opcode = SL_OP_THROW;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

NODE(sl_node_base_t, yada_yada)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_YADA_YADA;
    emit(cs, insn);
    (void)dest;
    (void)node;
}

NODE(sl_node__register_t, _register)
{
    sl_vm_insn_t insn;
    insn.opcode = SL_OP_MOV;
    emit(cs, insn);
    insn.uint = node->reg;
    emit(cs, insn);
    insn.uint = dest;
    emit(cs, insn);
}

#define COMPILE(type, caps, name) case SL_NODE_##caps: compile_##name(cs, (type*)node, dest); return;

static void
compile_node(sl_compile_state_t* cs, sl_node_base_t* node, size_t dest)
{
    sl_vm_insn_t insn;
    if(node->line != cs->last_line && node->line != 0) {
        cs->last_line = node->line;
        insn.opcode = SL_OP_LINE_TRACE;
        emit(cs, insn);
        insn.uint = node->line;
        emit(cs, insn);
    }
    switch(node->type) {
        COMPILE(sl_node_seq_t,           SEQ,            seq);
        COMPILE(sl_node_raw_t,           RAW,            raw);
        COMPILE(sl_node_echo_t,          ECHO,           echo);
        COMPILE(sl_node_echo_t,          ECHO_RAW,       echo_raw);
        COMPILE(sl_node_var_t,           VAR,            var);
        COMPILE(sl_node_var_t,           IVAR,           ivar);
        COMPILE(sl_node_var_t,           CVAR,           cvar);
        COMPILE(sl_node_immediate_t,     IMMEDIATE,      immediate);
        COMPILE(sl_node_base_t,          SELF,           self);
        COMPILE(sl_node_class_t,         CLASS,          class);
        COMPILE(sl_node_def_t,           DEF,            def);
        COMPILE(sl_node_lambda_t,        LAMBDA,         lambda);
        COMPILE(sl_node_try_t,           TRY,            try);
        COMPILE(sl_node_if_t,            IF,             if);
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
        COMPILE(sl_node__register_t,     _REGISTER,      _register);
    }
    sl_throw_message(cs->vm, "Unknown node type in compile_node");
}

sl_vm_section_t*
sl_compile(sl_vm_t* vm, sl_node_base_t* ast, uint8_t* filename)
{
    sl_vm_insn_t insn;
    sl_compile_state_t cs;
    init_compile_state(&cs, vm, NULL, 1);
    cs.section->filename = filename;
    cs.section->name = sl_intern(vm, "<main>");
    
    insn.opcode = SL_OP_IMMEDIATE;
    emit(&cs, insn);
    insn.imm = vm->lib.nil;
    emit(&cs, insn);
    insn.uint = 0;
    emit(&cs, insn);
    
    compile_node(&cs, ast, 0);
    
    insn.opcode = SL_OP_RETURN;
    emit(&cs, insn);
    insn.uint = 0;
    emit(&cs, insn);
    
    return cs.section;
}
