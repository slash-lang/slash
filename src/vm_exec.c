#include "vm_exec.inc"

VM_BEGIN

/*  0: RAW
    1: <imm> */
INSTRUCTION(SL_OP_RAW, {
    sl_response_write(vm, NEXT_IMM());
});

/*  0: ECHO
    1: <reg> */
INSTRUCTION(SL_OP_ECHO, {
    SLVAL tmp = sl_to_s(vm, NEXT_REG());
    tmp = sl_string_html_escape(vm, tmp);
    sl_response_write(vm, tmp);
});

/*  0: ECHO_RAW
    1: <reg> */
INSTRUCTION(SL_OP_ECHO_RAW, {
    SLVAL tmp = sl_to_s(vm, NEXT_REG());
    sl_response_write(vm, tmp);
});

/*  0: NOT
    1: <reg:src>
    2: <reg:dest> */
INSTRUCTION(SL_OP_NOT, {
    SLVAL tmp = NEXT_REG();
    if(sl_is_truthy(tmp)) {
        NEXT_REG() = V_FALSE;
    } else {
        NEXT_REG() = V_TRUE;
    }
});

/*  0: MOV
    1: <reg:src>
    2: <reg:dest> */
INSTRUCTION(SL_OP_MOV, {
    SLVAL tmp = NEXT_REG();
    NEXT_REG() = tmp;
});

/*  0: GET_OUTER
    1: <uint:frame_count>
    2: <uint:register_index>
    3: <reg:local> */
INSTRUCTION(SL_OP_GET_OUTER, {
    size_t frame_count = NEXT_UINT();
    sl_vm_exec_ctx_t* parent_ctx = ctx;
    while(frame_count--) {
        parent_ctx = parent_ctx->parent;
    }
    size_t register_index = NEXT_UINT();
    NEXT_REG() = parent_ctx->registers[register_index];
});

/*  0: GET_GLOBAL
    1: <str:var_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_GLOBAL, {
    sl_string_t* var_name = NEXT_STR();
    NEXT_REG() = sl_get_global2(vm, var_name);
});

/*  0: GET_IVAR
    1: <imm:var_name>
    2: <reg:dst> */
INSTRUCTION(SL_OP_GET_IVAR, {
    sl_string_t* var_name = NEXT_STR();
    NEXT_REG() = sl_get_ivar(vm, ctx->self, var_name);
});

/*  0: GET_CVAR
    1: <str:var_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_CVAR, {
    sl_string_t* var_name = NEXT_STR();
    NEXT_REG() = sl_get_cvar(vm, ctx->self, var_name);
});

/*  0: GET_CONST
    1: <str:const_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_CONST, {
    SLVAL const_name = NEXT_IMM();
    sl_vm_exec_ctx_t* lookup_ctx = ctx;
    while(lookup_ctx) {
        if(sl_class_has_const2(vm, lookup_ctx->self, const_name)) {
            NEXT_REG() = sl_class_get_const2(vm, lookup_ctx->self, const_name);
            break;
        }
        lookup_ctx = lookup_ctx->parent;
    }
    if(!lookup_ctx) {
        /* force exception */
        sl_class_get_const2(vm, ctx->self, const_name);
    }
});

/*  0: GET_OBJECT_CONST
    1: <reg:object>
    2: <str:const_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_GET_OBJECT_CONST, {
    SLVAL obj = NEXT_REG();
    SLVAL const_name = NEXT_IMM();
    NEXT_REG() = sl_class_get_const2(vm, obj, const_name);
});

/*  0: SET_OUTER
    1: <uint:frame_count>
    2: <uint:register_index>
    3: <reg:value> */
INSTRUCTION(SL_OP_SET_OUTER, {
    size_t frame_count = NEXT_UINT();
    sl_vm_exec_ctx_t* lookup_ctx = ctx;
    while(frame_count--) {
        lookup_ctx = lookup_ctx->parent;
    }
    size_t register_index = NEXT_UINT();
    lookup_ctx->registers[register_index] = NEXT_REG();
});

/*  0: SET_GLOBAL
    1: <str:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_GLOBAL, {
    sl_string_t* var_name = NEXT_STR();
    sl_set_global2(vm, var_name, NEXT_REG());
});

/*  0: SET_IVAR
    1: <imm:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_IVAR, {
    sl_string_t* var_name = NEXT_STR();
    sl_set_ivar(vm, ctx->self, var_name, NEXT_REG());
});

/*  0: SET_CVAR
    1: <str:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CVAR, {
    sl_string_t* var_name = NEXT_STR();
    sl_set_cvar(vm, ctx->self, var_name, NEXT_REG());
});

/*  0: SET_CONST
    1: <str:const_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CONST, {
    SLVAL const_name = NEXT_IMM();
    sl_class_set_const2(vm, ctx->self, const_name, NEXT_REG());
});

/*  0: SET_OBJECT_CONST
    1: <reg:object>
    2: <str:const_name>
    3: <reg:value> */
INSTRUCTION(SL_OP_SET_OBJECT_CONST, {
    SLVAL obj = NEXT_REG();
    SLVAL const_name = NEXT_IMM();
    sl_class_set_const2(vm, obj, const_name, NEXT_REG());
});

/*  0: IMMEDIATE
    1: <imm:value>
    2: <reg:dest> */
INSTRUCTION(SL_OP_IMMEDIATE, {
    SLVAL imm = NEXT_IMM();
    NEXT_REG() = imm;
});

/*  0: SEND
    1: <reg:object>
    2: <str:name>
    3: <reg:first_arg> // arguments must be in a contiguous register region
    4: <uint:arg_count>
    5: <reg:dest> */
INSTRUCTION(SL_OP_SEND, {
    SLVAL receiver = NEXT_REG();
    SLVAL id = NEXT_IMM();
    SLVAL* argv = &NEXT_REG();
    size_t argc = NEXT_UINT();
    /* @TODO: backtrace stuff */
    NEXT_REG() = sl_send2(vm, receiver, id, argc, argv);
});

/*  0: JUMP
    1: <uint:ip> */
INSTRUCTION(SL_OP_JUMP, {
    size_t new_ip = NEXT_UINT();
    ip = new_ip;
});

/*  0: JUMP_IF
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_IF, {
    size_t branch_ip = NEXT_UINT();
    SLVAL val = NEXT_REG();
    if(sl_is_truthy(val)) {
        ip = branch_ip;
    }
});

/*  0: JUMP_UNLESS
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_UNLESS, {
    size_t branch_ip = NEXT_UINT();
    SLVAL val = NEXT_REG();
    if(!sl_is_truthy(val)) {
        ip = branch_ip;
    }
});

/*  0: CLASS
    1: <str:name>
    2: <reg:extends>
    3: <section:body>
    4: <reg:dest> */
INSTRUCTION(SL_OP_CLASS, {
    SLVAL class_name = NEXT_IMM();
    SLVAL super_class = NEXT_REG();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_class(ctx, class_name, super_class, body);
});

/*  0: DEFINE
    1: <str:name>
    2: <section:body>
    3: <reg:dest> */
INSTRUCTION(SL_OP_DEFINE, {
    SLVAL id = NEXT_IMM();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_method(ctx, id, body);
});

/*  0: DEFINE
    1: <reg:on>
    2: <str:name>
    3: <section:body>
    4: <reg:dest> */
INSTRUCTION(SL_OP_DEFINE_ON, {
    SLVAL obj = NEXT_REG();
    SLVAL id = NEXT_IMM();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_singleton_method(ctx, obj, id, body);
});

/*  0: LAMBDA
    1: <section:body>
    2: <reg:dest> */
INSTRUCTION(SL_OP_LAMBDA, {
    SLVAL lambda = sl_make_lambda(NEXT_SECTION(), ctx);
    NEXT_REG() = lambda;
});

/*  0: SELF
    1: <reg:dest> */
INSTRUCTION(SL_OP_SELF, {
    NEXT_REG() = ctx->self;
});

/*  0: ARRAY
    1: <uint:count>
    2: <reg:first_element>
    3: <reg:dest> */
INSTRUCTION(SL_OP_ARRAY, {
    size_t count = NEXT_UINT();
    SLVAL* elements = &NEXT_REG();
    NEXT_REG() = sl_make_array(vm, count, elements);
});

/*  0: ARRAY_DUMP
    1: <reg:array>
    2: <uint:count>
    3: <reg:first_element> */
INSTRUCTION(SL_OP_ARRAY_DUMP, {
    SLVAL ary = NEXT_REG();
    size_t count = NEXT_UINT();
    SLVAL* registers = &NEXT_REG();
    if(sl_is_a(vm, ary, vm->lib.Array)) {
        for(size_t j = 0; j < count; j++) {
            registers[j] = sl_array_get(vm, ary, j);
        }
    } else {
        if(count) {
            registers[0] = ary;
        }
        for(size_t j = 1; j < count; j++) {
            registers[j] = V_NIL;
        }
    }
});

/*  0: DICT
    1: <uint:count>
    2: <reg:first_element>
    3: <reg:dest> */
INSTRUCTION(SL_OP_DICT, {
    size_t count = NEXT_UINT();
    SLVAL* key_values = &NEXT_REG();
    NEXT_REG() = sl_make_dict(vm, count, key_values);
});

/*  0: RETURN
    1: <reg:value> */
INSTRUCTION(SL_OP_RETURN, {
    /* @TODO: there may be stuff to clean up in future */
    return NEXT_REG();
});

/*  0: RANGE
    1: <reg:from>
    2: <reg:to>
    3: <reg:dest> */
INSTRUCTION(SL_OP_RANGE, {
    SLVAL left = NEXT_REG();
    SLVAL right = NEXT_REG();
    NEXT_REG() = sl_make_range(vm, left, right);
});

/*  0: RANGE_EX
    1: <reg:from>
    2: <reg:to>
    3: <reg:dest> */
INSTRUCTION(SL_OP_RANGE_EX, {
    SLVAL left = NEXT_REG();
    SLVAL right = NEXT_REG();
    NEXT_REG() = sl_make_range_exclusive(vm, left, right);
});

/*  0: LINE
    1: <uint:line> */
INSTRUCTION(SL_OP_LINE_TRACE, {
    line = NEXT_UINT();
});

/*  0: ABORT */
INSTRUCTION(SL_OP_ABORT, {
    sl_throw_message(vm, "VM abort");
});

VM_END
