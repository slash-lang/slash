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
    tmp = sl_to_s(vm, NEXT_REG());
    tmp = sl_string_html_escape(vm, tmp);
    sl_response_write(vm, tmp);
});

/*  0: ECHO_RAW
    1: <reg> */
INSTRUCTION(SL_OP_ECHO_RAW, {
    tmp = sl_to_s(vm, NEXT_REG());
    sl_response_write(vm, tmp);
});

/*  0: NOT
    1: <reg:src>
    2: <reg:dest> */
INSTRUCTION(SL_OP_NOT, {
    tmp = NEXT_REG();
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
    tmp = NEXT_REG();
    NEXT_REG() = tmp;
});

/*  0: GET_OUTER
    1: <uint:frame_count>
    2: <uint:register_index>
    3: <reg:local> */
INSTRUCTION(SL_OP_GET_OUTER, {
    tmp_uint = NEXT_UINT();
    tmp_ctx = ctx;
    while(tmp_uint--) {
        tmp_ctx = tmp_ctx->parent;
    }
    tmp_uint = NEXT_UINT();
    NEXT_REG() = tmp_ctx->registers[tmp_uint];
});

/*  0: GET_GLOBAL
    1: <str:var_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_GLOBAL, {
    tmp_str = NEXT_STR();
    NEXT_REG() = sl_get_global2(vm, tmp_str);
});

/*  0: GET_IVAR
    1: <imm:var_name>
    2: <reg:dst> */
INSTRUCTION(SL_OP_GET_IVAR, {
    tmp_str = NEXT_STR();
    NEXT_REG() = sl_get_ivar(vm, ctx->self, tmp_str);
});

/*  0: GET_CVAR
    1: <str:var_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_CVAR, {
    tmp_str = NEXT_STR();
    NEXT_REG() = sl_get_cvar(vm, ctx->self, tmp_str);
});

/*  0: GET_CONST
    1: <str:const_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_CONST, {
    tmp = NEXT_IMM();
    tmp_ctx = ctx;
    while(tmp_ctx) {
        if(sl_class_has_const2(vm, tmp_ctx->self, tmp)) {
            NEXT_REG() = sl_class_get_const2(vm, tmp_ctx->self, tmp);
            break;
        }
        tmp_ctx = tmp_ctx->parent;
    }
    if(!tmp_ctx) {
        /* force exception */
        sl_class_get_const2(vm, ctx->self, tmp);
    }
});

/*  0: GET_OBJECT_CONST
    1: <reg:object>
    2: <str:const_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_GET_OBJECT_CONST, {
    tmp = NEXT_REG();
    tmp_str = NEXT_STR();
    NEXT_REG() = sl_class_get_const2(vm, tmp, sl_make_ptr((sl_object_t*)tmp_str));
});

/*  0: SET_OUTER
    1: <uint:frame_count>
    2: <uint:register_index>
    3: <reg:value> */
INSTRUCTION(SL_OP_SET_OUTER, {
    tmp_uint = NEXT_UINT();
    tmp_ctx = ctx;
    while(tmp_uint--) {
        tmp_ctx = tmp_ctx->parent;
    }
    tmp_uint = NEXT_UINT();
    tmp_ctx->registers[tmp_uint] = NEXT_REG();
});

/*  0: SET_GLOBAL
    1: <str:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_GLOBAL, {
    tmp_str = NEXT_STR();
    sl_set_global2(vm, tmp_str, NEXT_REG());
});

/*  0: SET_IVAR
    1: <imm:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_IVAR, {
    tmp_str = NEXT_STR();
    sl_set_ivar(vm, ctx->self, tmp_str, NEXT_REG());
});

/*  0: SET_CVAR
    1: <str:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CVAR, {
    tmp_str = NEXT_STR();
    sl_set_cvar(vm, ctx->self, tmp_str, NEXT_REG());
});

/*  0: SET_CONST
    1: <str:const_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CONST, {
    tmp = NEXT_IMM();
    sl_class_set_const2(vm, ctx->self, tmp, NEXT_REG());
});

/*  0: SET_OBJECT_CONST
    1: <reg:object>
    2: <str:const_name>
    3: <reg:value> */
INSTRUCTION(SL_OP_SET_OBJECT_CONST, {
    tmp = NEXT_REG();
    tmp_str = NEXT_STR();
    sl_class_set_const2(vm, tmp, sl_make_ptr((sl_object_t*)tmp_str), NEXT_REG());
});

/*  0: IMMEDIATE
    1: <imm:value>
    2: <reg:dest> */
INSTRUCTION(SL_OP_IMMEDIATE, {
    tmp = NEXT_IMM();
    NEXT_REG() = tmp;
});

/*  0: SEND
    1: <reg:object>
    2: <str:name>
    3: <reg:first_arg> // arguments must be in a contiguous register region
    4: <uint:arg_count>
    5: <reg:dest> */
INSTRUCTION(SL_OP_SEND, {
    tmp = NEXT_REG();
    tmp_str = NEXT_STR();
    tmp_p = &NEXT_REG();
    tmp_uint = NEXT_UINT();
    /* @TODO: backtrace stuff */
    NEXT_REG() = sl_send2(vm, tmp, sl_make_ptr((sl_object_t*)tmp_str), tmp_uint, tmp_p);
});

/*  0: JUMP
    1: <uint:ip> */
INSTRUCTION(SL_OP_JUMP, {
    tmp_uint = NEXT_UINT();
    ip = tmp_uint;
});

/*  0: JUMP_IF
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_IF, {
    tmp_uint = NEXT_UINT();
    tmp = NEXT_REG();
    if(sl_is_truthy(tmp)) {
        ip = tmp_uint;
    }
});

/*  0: JUMP_UNLESS
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_UNLESS, {
    tmp_uint = NEXT_UINT();
    tmp = NEXT_REG();
    if(!sl_is_truthy(tmp)) {
        ip = tmp_uint;
    }
});

/*  0: CLASS
    1: <str:name>
    2: <reg:extends>
    3: <section:body>
    4: <reg:dest> */
INSTRUCTION(SL_OP_CLASS, {
    #define name tmp
    #define extends tmp2
    name = NEXT_IMM();
    extends = NEXT_REG();
    tmp_section = NEXT_SECTION();
    NEXT_REG() = sl_vm_define_class(ctx, name, extends, tmp_section);
    #undef name
    #undef extends
});

/* @TODO DEFINE, DEFINE_ON */
INSTRUCTION(SL_OP_DEFINE, {
    sl_throw_message(vm, "DEFINE opcode not yet implemented");
});
INSTRUCTION(SL_OP_DEFINE_ON, {
    sl_throw_message(vm, "DEFINE_ON opcode not yet implemented");
});

/*  0: LAMBDA
    1: <section:body>
    2: <reg:dest> */
INSTRUCTION(SL_OP_LAMBDA, {
    tmp = sl_make_lambda(NEXT_SECTION(), ctx);
    NEXT_REG() = tmp;
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
    tmp_uint = NEXT_UINT();
    tmp_p = &NEXT_REG();
    NEXT_REG() = sl_make_array(vm, tmp_uint, tmp_p);
});

/*  0: DICT
    1: <uint:count>
    2: <reg:first_element>
    3: <reg:dest> */
INSTRUCTION(SL_OP_DICT, {
    tmp_uint = NEXT_UINT();
    tmp_p = &NEXT_REG();
    NEXT_REG() = sl_make_dict(vm, tmp_uint, tmp_p);
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
    tmp = NEXT_REG();
    tmp = sl_make_range(vm, tmp, NEXT_REG());
    NEXT_REG() = tmp;
});

/*  0: RANGE_EX
    1: <reg:from>
    2: <reg:to>
    3: <reg:dest> */
INSTRUCTION(SL_OP_RANGE_EX, {
    tmp = NEXT_REG();
    tmp = sl_make_range_exclusive(vm, tmp, NEXT_REG());
    NEXT_REG() = tmp;
});

VM_END
