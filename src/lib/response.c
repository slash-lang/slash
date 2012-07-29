#include "lib/response.h"
#include "lib/dict.h"

static int Response_;
static int Response_opts;

void
sl_response_set_opts(sl_vm_t* vm, sl_response_opts_t* opts)
{
    sl_vm_store_put(vm, &Response_opts, sl_make_ptr((sl_object_t*)opts));
}

static sl_response_opts_t*
response(sl_vm_t* vm)
{
    return (sl_response_opts_t*)sl_get_ptr(sl_vm_store_get(vm, &Response_opts));
}

static SLVAL
response_write(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_response_opts_t* resp = response(vm);
    size_t i;
    sl_string_t* str;
    (void)self;
    if(resp->buffered) {
        for(i = 0; i < argc; i++) {
            resp->output_buffer = sl_string_concat(vm, resp->output_buffer, sl_to_s(vm, argv[i]));
        }
    } else {
        for(i = 0; i < argc; i++) {
            str = (sl_string_t*)sl_get_ptr(sl_to_s(vm, argv[i]));
            resp->write(vm, (char*)str->buff, str->buff_len);
        }
    }
    return vm->lib.nil;
}

static SLVAL
response_unbuffer(sl_vm_t* vm)
{
    sl_response_opts_t* resp = response(vm);
    sl_string_t* str;
    if(!resp->buffered) {
        return vm->lib._false;
    }
    str = (sl_string_t*)sl_get_ptr(resp->output_buffer);
    if(str->buff_len) {
        resp->write(vm, (char*)str->buff, str->buff_len);
    }
    return vm->lib._true;
}

void
sl_init_response(sl_vm_t* vm)
{
    SLVAL Response = sl_new(vm, vm->lib.Object, 0, NULL);
    sl_vm_store_put(vm, &Response_, Response);
    
    sl_define_singleton_method(vm, Response, "write", -2, response_write);
    sl_define_method(vm, vm->lib.Object, "print", -2, response_write);
    
    sl_define_method(vm, vm->lib.Object, "unbuffer", 0, response_unbuffer);
    
    sl_class_set_const(vm, vm->lib.Object, "Response", Response);
}
