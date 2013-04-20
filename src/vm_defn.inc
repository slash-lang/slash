/*  0: RAW
    1: <imm> */
INSTRUCTION(SL_OP_RAW, {
    sl_response_write(vm, NEXT_IMM());
})

/*  0: ECHO
    1: <reg> */
INSTRUCTION(SL_OP_ECHO, {
    SLVAL tmp = sl_to_s(vm, NEXT_REG());
    tmp = sl_string_html_escape(vm, tmp);
    sl_response_write(vm, tmp);
})

/*  0: ECHO_RAW
    1: <reg> */
INSTRUCTION(SL_OP_ECHO_RAW, {
    SLVAL tmp = sl_to_s(vm, NEXT_REG());
    sl_response_write(vm, tmp);
})

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
})

/*  0: MOV
    1: <reg:src>
    2: <reg:dest> */
INSTRUCTION(SL_OP_MOV, {
    SLVAL tmp = NEXT_REG();
    NEXT_REG() = tmp;
})

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
})

/*  0: GET_IVAR
    1: <id:var_name>
    2: <reg:dst> */
INSTRUCTION(SL_OP_GET_IVAR, {
    SLID var_name = NEXT_ID();
    NEXT_REG() = sl_get_ivar(vm, ctx->self, var_name);
})

/*  0: GET_CVAR
    1: <id:var_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_GET_CVAR, {
    SLID var_name = NEXT_ID();
    NEXT_REG() = sl_get_cvar(vm, ctx->self, var_name);
})

/*  0: GET_CONST
    1: <icc:inline_cache>
    2: <id:const_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_GET_CONST, {
    sl_vm_inline_constant_cache_t* icc = NEXT_ICC();
    SLID const_name = NEXT_ID();
    if(sl_likely(icc->state == vm->state_constant)) {
        NEXT_REG() = icc->value;
    } else {
        sl_vm_exec_ctx_t* lookup_ctx = ctx;
        while(lookup_ctx) {
            if(sl_class_has_const2(vm, lookup_ctx->self, const_name)) {
                icc->state = vm->state_constant;
                icc->value = sl_class_get_const2(vm, lookup_ctx->self, const_name);
                NEXT_REG() = icc->value;
                break;
            }
            lookup_ctx = lookup_ctx->parent;
        }
        if(!lookup_ctx) {
            /* force exception */
            sl_class_get_const2(vm, ctx->self, const_name);
        }
    }
})

/*  0: GET_OBJECT_CONST
    1: <reg:object>
    2: <id:const_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_GET_OBJECT_CONST, {
    SLVAL obj = NEXT_REG();
    SLID const_name = NEXT_ID();
    NEXT_REG() = sl_class_get_const2(vm, obj, const_name);
})

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
})

/*  0: SET_IVAR
    1: <id:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_IVAR, {
    SLID var_name = NEXT_ID();
    sl_set_ivar(vm, ctx->self, var_name, NEXT_REG());
})

/*  0: SET_CVAR
    1: <id:var_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CVAR, {
    SLID var_name = NEXT_ID();
    sl_set_cvar(vm, ctx->self, var_name, NEXT_REG());
})

/*  0: SET_CONST
    1: <id:const_name>
    2: <reg:value> */
INSTRUCTION(SL_OP_SET_CONST, {
    SLID const_name = NEXT_ID();
    sl_class_set_const2(vm, ctx->self, const_name, NEXT_REG());
})

/*  0: SET_OBJECT_CONST
    1: <reg:object>
    2: <id:const_name>
    3: <reg:value> */
INSTRUCTION(SL_OP_SET_OBJECT_CONST, {
    SLVAL obj = NEXT_REG();
    SLID const_name = NEXT_ID();
    sl_class_set_const2(vm, obj, const_name, NEXT_REG());
})

/*  0: IMMEDIATE
    1: <imm:value>
    2: <reg:dest> */
INSTRUCTION(SL_OP_IMMEDIATE, {
    SLVAL imm = NEXT_IMM();
    NEXT_REG() = imm;
})

/*  0: SEND
    1: <reg:object>
    2: <ic:inline_cache>
    3: <reg:first_arg> // arguments must be in a contiguous register region
    4: <reg:dest> */
INSTRUCTION(SL_OP_SEND, {
    SLVAL receiver = NEXT_REG();
    sl_vm_inline_method_cache_t* imc = NEXT_IMC();
    SLVAL* argv = &NEXT_REG();

    if(sl_likely(imc->state == vm->state_method && imc->klass.i == sl_real_class_of(vm, receiver).i)) {
        NEXT_REG() = imc->call(vm, imc, receiver, argv);
    } else {
        NEXT_REG() = sl_imc_setup_call(vm, imc, receiver, argv);
    }
})

/*  0: JUMP
    1: <uint:ip> */
INSTRUCTION(SL_OP_JUMP, {
    size_t new_ip = NEXT_UINT();
    ip = new_ip;
})

/*  0: JUMP_IF
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_IF, {
    size_t branch_ip = NEXT_UINT();
    SLVAL val = NEXT_REG();
    if(sl_is_truthy(val)) {
        ip = branch_ip;
    }
})

/*  0: JUMP_UNLESS
    1: <uint:ip>
    2: <reg:condition> */
INSTRUCTION(SL_OP_JUMP_UNLESS, {
    size_t branch_ip = NEXT_UINT();
    SLVAL val = NEXT_REG();
    if(!sl_is_truthy(val)) {
        ip = branch_ip;
    }
})

/*  0: CLASS
    1: <id:name>
    2: <reg:extends>
    3: <section:body>
    4: <reg:dest> */
INSTRUCTION(SL_OP_CLASS, {
    SLID class_name = NEXT_ID();
    SLVAL super_class = NEXT_REG();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_class(ctx, class_name, super_class, body);
})

/*  0: DEFINE
    1: <id:name>
    2: <section:body>
    3: <reg:dest> */
INSTRUCTION(SL_OP_DEFINE, {
    SLID id = NEXT_ID();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_method(ctx, id, body);
})

/*  0: DEFINE_ON
    1: <reg:on>
    2: <id:name>
    3: <section:body>
    4: <reg:dest> */
INSTRUCTION(SL_OP_DEFINE_ON, {
    SLVAL obj = NEXT_REG();
    SLID id = NEXT_ID();
    sl_vm_section_t* body = NEXT_SECTION();
    NEXT_REG() = vm_helper_define_singleton_method(ctx, obj, id, body);
})

/*  0: LAMBDA
    1: <section:body>
    2: <reg:dest> */
INSTRUCTION(SL_OP_LAMBDA, {
    SLVAL lambda = sl_make_lambda(NEXT_SECTION(), ctx);
    NEXT_REG() = lambda;
})

/*  0: SELF
    1: <reg:dest> */
INSTRUCTION(SL_OP_SELF, {
    NEXT_REG() = ctx->self;
})

/*  0: ARRAY
    1: <uint:count>
    2: <reg:first_element>
    3: <reg:dest> */
INSTRUCTION(SL_OP_ARRAY, {
    size_t count = NEXT_UINT();
    SLVAL* elements = &NEXT_REG();
    NEXT_REG() = sl_make_array(vm, count, elements);
})

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
})

/*  0: DICT
    1: <uint:count>
    2: <reg:first_element>
    3: <reg:dest> */
INSTRUCTION(SL_OP_DICT, {
    size_t count = NEXT_UINT();
    SLVAL* key_values = &NEXT_REG();
    NEXT_REG() = sl_make_dict(vm, count, key_values);
})

/*  0: RETURN
    1: <reg:value> */
INSTRUCTION(SL_OP_RETURN, {
    vm->call_stack = call_frame.prev;
    return NEXT_REG();
})

/*  0: RANGE
    1: <reg:from>
    2: <reg:to>
    3: <reg:dest> */
INSTRUCTION(SL_OP_RANGE, {
    SLVAL left = NEXT_REG();
    SLVAL right = NEXT_REG();
    NEXT_REG() = sl_make_range(vm, left, right);
})

/*  0: RANGE_EX
    1: <reg:from>
    2: <reg:to>
    3: <reg:dest> */
INSTRUCTION(SL_OP_RANGE_EX, {
    SLVAL left = NEXT_REG();
    SLVAL right = NEXT_REG();
    NEXT_REG() = sl_make_range_exclusive(vm, left, right);
})

/*  0: LINE_TRACE
    1: <uint:line> */
INSTRUCTION(SL_OP_LINE_TRACE, {
    line = NEXT_UINT();
})

/*  0: ABORT */
INSTRUCTION(SL_OP_ABORT, {
    sl_throw_message(vm, "VM abort");
})

/*  0: THROW
    1: <reg:exception> */
INSTRUCTION(SL_OP_THROW, {
    sl_throw(vm, NEXT_REG());
})

/*  0: TRY
    1: <uint:catch_ip> */
INSTRUCTION(SL_OP_TRY, {
    sl_vm_exception_handler_t* eh = sl_alloc(vm->arena, sizeof(sl_vm_exception_handler_t));
    eh->prev = exception_handler;
    eh->catch_ip = NEXT_UINT();
    exception_handler = eh;
})

/*  0: END_TRY */
INSTRUCTION(SL_OP_END_TRY, {
    exception_handler = exception_handler->prev;
})

/*  0: CATCH
    1: <reg:exception_dest> */
INSTRUCTION(SL_OP_CATCH, {
    NEXT_REG() = catch_frame.as.handler_frame.value;
})

/*  0: YADA_YADA */
INSTRUCTION(SL_OP_YADA_YADA, {
    sl_throw_message2(vm, vm->lib.NotImplementedError, "Not Implemented");
})

/*  0: BIND_METHOD
    1: <reg:receiver>
    2: <id:method_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_BIND_METHOD, {
    SLVAL recv = NEXT_REG();
    SLID method_name = NEXT_ID();
    sl_method_t* method = sl_lookup_method(vm, recv, method_name);
    if(sl_likely(method != NULL)) {
        NEXT_REG() = sl_method_bind(vm, sl_make_ptr((sl_object_t*)method), recv);
    } else {
        sl_error(vm, vm->lib.NoMethodError, "No such method %QI", method_name);
    }
})

/*  0: USE
    1: <reg:klass>
    2: <id:const_name>
    3: <reg:dest> */
INSTRUCTION(SL_OP_USE, {
    SLVAL klass = NEXT_REG();
    SLID id = NEXT_ID();
    if(sl_unlikely(sl_get_primitive_type(klass) != SL_T_CLASS)) {
        klass = sl_class_of(vm, klass);
    }
    if(sl_unlikely(!sl_class_has_const2(vm, klass, id))) {
        if(!sl_class_has_full_path(vm, klass)) {
            sl_throw_message2(vm, vm->lib.NameError, "Unnamed classes cannot be used in use expressions");
        }
        SLVAL require_path = sl_class_file_path(vm, klass);
        require_path = sl_string_concat(vm, require_path, sl_make_cstring(vm, "/"));
        require_path = sl_string_concat(vm, require_path, sl_camel_case_to_underscore(vm, sl_id_to_string(vm, id)));
        sl_require(vm, sl_to_cstr(vm, require_path));
    }
    NEXT_REG() = sl_class_get_const2(vm, klass, id);
})

/*  0: USE_TOP_LEVEL
    1: <id:const_name>
    2: <reg:dest> */
INSTRUCTION(SL_OP_USE_TOP_LEVEL, {
    SLID id = NEXT_ID();
    if(sl_unlikely(!sl_class_has_const2(vm, vm->lib.Object, id))) {
        SLVAL require_path = sl_camel_case_to_underscore(vm, sl_id_to_string(vm, id));
        sl_require(vm, sl_to_cstr(vm, require_path));
    }
    NEXT_REG() = sl_class_get_const2(vm, vm->lib.Object, id);
})

/*  0: BUILD_STRING
    1: <reg:base_reg>
    2: <uint:reg_count>
    3: <reg:dest> */
INSTRUCTION(SL_OP_BUILD_STRING, {
    SLVAL* base = &NEXT_REG();
    size_t count = NEXT_UINT();
    NEXT_REG() = vm_helper_build_string(vm, base, count);
})
