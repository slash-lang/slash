#include <gc.h>
#include <string.h>
#include "lib/response.h"
#include "lib/dict.h"

static int Response_;
static int Response_opts;

typedef struct {
    int status;
    int descriptive_error_page;
    sl_response_key_value_t* headers;
    size_t header_cap;
    size_t header_count;
    int buffered;
    SLVAL output_buffer;
    void(*write)(sl_vm_t*,char*,size_t);
}
sl_response_internal_opts_t;

void
sl_response_set_opts(sl_vm_t* vm, sl_response_opts_t* opts)
{
    sl_response_internal_opts_t* iopts = GC_MALLOC(sizeof(sl_response_internal_opts_t));
    iopts->status        = 200;
    iopts->buffered      = opts->buffered;
    iopts->output_buffer = sl_make_string(vm, NULL, 0);
    iopts->write         = opts->write;
    iopts->header_cap    = 2;
    iopts->header_count  = 0;
    iopts->headers       = GC_MALLOC(sizeof(sl_response_key_value_t) * iopts->header_cap);
    iopts->descriptive_error_page = opts->descriptive_error_page;
    sl_vm_store_put(vm, &Response_opts, sl_make_ptr((sl_object_t*)iopts));
}

static sl_response_internal_opts_t*
response(sl_vm_t* vm)
{
    return (sl_response_internal_opts_t*)sl_get_ptr(sl_vm_store_get(vm, &Response_opts));
}

static SLVAL
response_write(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_response_internal_opts_t* resp = response(vm);
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

SLVAL
sl_response_write(sl_vm_t* vm, SLVAL str)
{
    return response_write(vm, vm->lib.nil, 1, &str);
}

SLVAL
sl_response_flush(sl_vm_t* vm)
{
    sl_response_internal_opts_t* resp = response(vm);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(resp->output_buffer);
    if(str->buff_len) {
        resp->write(vm, (char*)str->buff, str->buff_len);
    }
    resp->output_buffer = sl_make_string(vm, NULL, 0);
    return vm->lib.nil;
}

static SLVAL
response_unbuffer(sl_vm_t* vm)
{
    sl_response_internal_opts_t* resp = response(vm);
    if(!resp->buffered) {
        return vm->lib._false;
    }
    sl_response_flush(vm);
    return vm->lib._true;
}

static SLVAL
response_set_header(sl_vm_t* vm, SLVAL self, SLVAL name, SLVAL value)
{
    sl_response_internal_opts_t* resp = response(vm);
    (void)self;
    sl_expect(vm, name, vm->lib.String);
    sl_expect(vm, value, vm->lib.String);
    if(resp->header_count >= resp->header_cap) {
        resp->header_cap *= 2;
        resp->headers = GC_REALLOC(resp->headers, sizeof(sl_response_key_value_t) * resp->header_cap);
    }
    resp->headers[resp->header_count].name = sl_to_cstr(vm, name);
    resp->headers[resp->header_count].value = sl_to_cstr(vm, value);
    resp->header_count++;
    return vm->lib.nil;
}

struct get_headers_iter_state {
    sl_response_key_value_t* headers;
    size_t at;
};

sl_response_key_value_t*
sl_response_get_headers(sl_vm_t* vm, size_t* count)
{
    *count = response(vm)->header_count;
    return response(vm)->headers;
}

int
sl_response_get_status(sl_vm_t* vm)
{
    return response(vm)->status;
}

static SLVAL
response_status(sl_vm_t* vm)
{
    return sl_make_int(vm, response(vm)->status);
}

static SLVAL
response_status_set(sl_vm_t* vm, SLVAL self, SLVAL status)
{
    (void)self;
    sl_expect(vm, status, vm->lib.Int);
    response(vm)->status = sl_get_int(status);
    return status;
}

static SLVAL
response_descriptive_error_pages(sl_vm_t* vm)
{
    if(response(vm)->descriptive_error_pages) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
response_descriptive_error_pages_set(sl_vm_t* vm, SLVAL self, SLVAL enabled)
{
    (void)self;
    response(vm)->descriptive_error_pages = sl_is_truthy(enabled);
    return enabled;
}

void
sl_init_response(sl_vm_t* vm)
{
    SLVAL Response = sl_new(vm, vm->lib.Object, 0, NULL);
    sl_vm_store_put(vm, &Response_, Response);
    
    sl_define_singleton_method(vm, Response, "write", -2, sl_response_write);
    sl_define_method(vm, vm->lib.Object, "print", -2, response_write);
    
    sl_define_method(vm, vm->lib.Object, "flush", 0, sl_response_flush);
    sl_define_method(vm, vm->lib.Object, "unbuffer", 0, response_unbuffer);
    sl_define_method(vm, vm->lib.Object, "set_header", 2, response_set_header);
    sl_define_method(vm, vm->lib.Object, "status", 0, response_status);
    sl_define_method(vm, vm->lib.Object, "status=", 1, response_status_set);
    
    sl_class_set_const(vm, vm->lib.Object, "Response", Response);
}
