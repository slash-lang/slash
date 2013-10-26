RAW(SLVAL imm)
{
    sl_response_write(vm, imm);
}

ECHO(REG str)
{
    str = sl_to_s(vm, str);
    str = sl_string_html_escape(vm, str);
    sl_response_write(vm, str);
}

ECHO_RAW(REG str)
{
    str = sl_to_s(vm, str);
    sl_response_write(vm, str);
}

NOT(REG src, REG* dest)
{
    if(sl_is_truthy(src)) {
        *dest = V_FALSE;
    } else {
        *dest = V_TRUE;
    }
}

MOV(REG src, REG* dest)
{
    *dest = src;
}

GET_OUTER(size_t frame_count, size_t register_index, REG* dest)
{
    sl_vm_exec_ctx_t* parent_ctx = ctx;
    while(frame_count--) {
        parent_ctx = parent_ctx->parent;
    }
    *dest = parent_ctx->registers[register_index];
}

GET_IVAR(SLID var_name, REG* dest)
{
    *dest = sl_get_ivar(vm, ctx->self, var_name);
}

GET_CVAR(SLID var_name, REG* dest)
{
    *dest = sl_get_cvar(vm, ctx->self, var_name);
}

GET_CONST(sl_vm_inline_constant_cache_t* icc, SLID const_name, REG* dest)
{
    if(sl_likely(icc->state == vm->state_constant)) {
        *dest = icc->value;
    } else {
        sl_vm_exec_ctx_t* lookup_ctx = ctx;
        while(lookup_ctx) {
            if(sl_class_has_const2(vm, lookup_ctx->self, const_name)) {
                icc->state = vm->state_constant;
                icc->value = sl_class_get_const2(vm, lookup_ctx->self, const_name);
                *dest = icc->value;
                break;
            }
            lookup_ctx = lookup_ctx->parent;
        }
        if(!lookup_ctx) {
            /* force exception */
            sl_class_get_const2(vm, ctx->self, const_name);
        }
    }
}

GET_OBJECT_CONST(REG object, SLID const_name, REG* dest)
{
    *dest = sl_class_get_const2(vm, object, const_name);
}

SET_OUTER(size_t frame_count, size_t register_index, REG value)
{
    sl_vm_exec_ctx_t* lookup_ctx = ctx;
    while(frame_count--) {
        lookup_ctx = lookup_ctx->parent;
    }
    lookup_ctx->registers[register_index] = value;
}

SET_IVAR(SLID var_name, REG value)
{
    sl_set_ivar(vm, ctx->self, var_name, value);
}

SET_CVAR(SLID var_name, REG value)
{
    sl_set_cvar(vm, ctx->self, var_name, value);
}

SET_CONST(SLID const_name, REG value)
{
    sl_class_set_const2(vm, ctx->self, const_name, value);
}

SET_OBJECT_CONST(REG object, SLID const_name, SLVAL value)
{
    sl_class_set_const2(vm, object, const_name, value);
}

IMMEDIATE(SLVAL value, REG* dest)
{
    *dest = value;
}

SEND(REG receiver, sl_vm_inline_method_cache_t* imc, REG* argv, REG* dest)
{
    if(sl_likely(imc->state == vm->state_method && imc->klass.i == sl_real_class_of(vm, receiver).i)) {
        *dest = imc->call(vm, imc, receiver, argv);
    } else {
        *dest = sl_imc_setup_call(vm, imc, receiver, argv);
    }
}

SEND_SELF(sl_vm_inline_method_cache_t* imc, REG* argv, REG* dest)
{
    if(sl_likely(imc->state == vm->state_method && imc->klass.i == sl_real_class_of(vm, ctx->self).i)) {
        *dest = imc->call(vm, imc, ctx->self, argv);
    } else {
        *dest = sl_imc_setup_call(vm, imc, ctx->self, argv);
    }
}

JUMP(size_t new_ip)
{
    ip = new_ip;
}

JUMP_IF(size_t branch_ip, REG val)
{
    if(sl_is_truthy(val)) {
        ip = branch_ip;
    }
}

JUMP_UNLESS(size_t branch_ip, REG val)
{
    if(!sl_is_truthy(val)) {
        ip = branch_ip;
    }
}

CLASS(SLID name, SLVAL doc, REG extends, sl_vm_section_t* section, REG* dest)
{
    *dest = vm_helper_define_class(ctx, name, doc, extends, section);
}

DEFINE(SLID name, SLVAL doc, sl_vm_section_t* section, REG* dest)
{
    *dest = vm_helper_define_method(ctx, name, doc, section);
}

DEFINE_ON(REG obj, SLID name, SLVAL doc, sl_vm_section_t* section, REG* dest)
{
    *dest = vm_helper_define_singleton_method(ctx, obj, name, doc, section);
}

LAMBDA(sl_vm_section_t* section, REG* dest)
{
    *dest = sl_make_lambda(section, ctx);
}

SELF(REG* dest)
{
    *dest = ctx->self;
}

ARRAY(size_t count, REG* elements, REG* dest)
{
    *dest = sl_make_array(vm, count, elements);
}

ARRAY_DUMP(REG ary, size_t count, REG* registers)
{
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
}

DICT(size_t count, REG* key_values, REG* dest)
{
    *dest = sl_make_dict(vm, count, key_values);
}


RETURN(REG value)
{
    vm->call_stack = call_frame.prev;
    return value;
}

RANGE_IN(REG left, REG right, REG* dest)
{
    *dest = sl_make_range_inclusive(vm, left, right);
}

RANGE_EX(REG left, REG right, REG* dest)
{
    *dest = sl_make_range_exclusive(vm, left, right);
}

LINE_TRACE(size_t new_line)
{
    line = new_line;
}

THROW(REG exception)
{
    sl_throw(vm, exception);
}

TRY(size_t catch_ip)
{
    sl_vm_exception_handler_t* eh = sl_alloc(vm->arena, sizeof(sl_vm_exception_handler_t));
    eh->prev = exception_handler;
    eh->catch_ip = catch_ip;
    exception_handler = eh;
}

END_TRY()
{
    exception_handler = exception_handler->prev;
}

CATCH(REG* dest)
{
    *dest = catch_frame.as.handler_frame.value;
}

YADA_YADA()
{
    sl_throw_message2(vm, vm->lib.NotImplementedError, "Not Implemented");
}

BIND_METHOD(REG recv, SLID method_name, REG* dest)
{
    sl_method_t* method = sl_lookup_method(vm, recv, method_name);
    if(sl_likely(method != NULL)) {
        *dest = sl_method_bind(vm, sl_make_ptr((sl_object_t*)method), recv);
    } else {
        sl_error(vm, vm->lib.NoMethodError, "No such method %QI", method_name);
    }
}

USE(REG klass, SLID id, REG* dest)
{
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
    *dest = sl_class_get_const2(vm, klass, id);
}

USE_TOP_LEVEL(SLID id, REG* dest)
{
    if(sl_unlikely(!sl_class_has_const2(vm, vm->lib.Object, id))) {
        SLVAL require_path = sl_camel_case_to_underscore(vm, sl_id_to_string(vm, id));
        sl_require(vm, sl_to_cstr(vm, require_path));
    }
    *dest = sl_class_get_const2(vm, vm->lib.Object, id);
}

BUILD_STRING(REG* base_reg, size_t size, REG* dest)
{
    *dest = vm_helper_build_string(vm, base_reg, size);
}
