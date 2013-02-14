#define CONST(opt) /* nothing */

#define TYPE_INT \
    curl_easy_check_error(vm, curl_easy_setopt(curl->curl, opt, sl_get_int(sl_expect(vm, argument, vm->lib.Int))));

#define TYPE_BOOL \
    curl_easy_check_error(vm, curl_easy_setopt(curl->curl, opt, sl_is_truthy(argument) ? 1 : 0));

#define TYPE_STRING \
    char* val = sl_to_cstr(vm, sl_expect(vm, argument, vm->lib.String)); \
    curl_easy_check_error(vm, curl_easy_setopt(curl->curl, opt, val)); \
    sl_array_push(vm, curl->gc_protect_array, 1, (void*)&val);

#define TYPE_BUFFER(actualopt, lenopt) \
    if(sl_get_primitive_type(argument) == SL_T_STRING) { \
        sl_string_t* str = sl_get_string(vm, argument); \
        curl_easy_check_error(vm, curl_easy_setopt(curl->curl, actualopt, str->buff)); \
        curl_easy_check_error(vm, curl_easy_setopt(curl->curl, lenopt, str->buff_len)); \
        sl_array_push(vm, curl->gc_protect_array, 1, &argument); \
    } else { \
        sl_frozen_buffer_t* buff = sl_buffer_get_frozen_copy(vm, argument); \
        curl_easy_check_error(vm, curl_easy_setopt(curl->curl, actualopt, buff->data)); \
        curl_easy_check_error(vm, curl_easy_setopt(curl->curl, lenopt, buff->len)); \
        sl_array_push(vm, curl->gc_protect_array, 1, (void*)&buff); \
    }

#define OPT(_opt, _argtype) case _opt: { _argtype } break;

static SLVAL
sl_curl_easy_setopt(sl_vm_t* vm, SLVAL self, SLVAL vopt, SLVAL argument)
{
    curl_easy_t* curl = get_curl_easy(vm, self);
    CURLoption opt = sl_get_int(sl_expect(vm, vopt, vm->lib.Int));
    switch(opt) {
        #include "curlopts.inc"
    default:
        sl_throw_message2(vm, vm->lib.ArgumentError, "Unknown curlopt");
    }
    return vm->lib.nil;
}

#undef OPT
#undef CONST
