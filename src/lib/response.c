#include <string.h>
#include <slash/lib/response.h>
#include <slash/lib/dict.h>

static int Response_;
static int Response_opts;

typedef struct {
    int status;
    int descriptive_error_pages;
    sl_response_key_value_t* headers;
    size_t header_cap;
    size_t header_count;
    int buffered;
    size_t output_cap;
    size_t output_len;
    SLVAL* output;
    void(*write)(sl_vm_t*,char*,size_t);
}
sl_response_internal_opts_t;

void
sl_response_set_opts(sl_vm_t* vm, sl_response_opts_t* opts)
{
    sl_response_internal_opts_t* iopts = sl_alloc(vm->arena, sizeof(sl_response_internal_opts_t));
    iopts->status        = 200;
    iopts->buffered      = opts->buffered;
    iopts->output_cap    = 4;
    iopts->output_len    = 0;
    iopts->output        = sl_alloc(vm->arena, sizeof(SLVAL) * iopts->output_cap);
    iopts->write         = opts->write;
    iopts->header_cap    = 2;
    iopts->header_count  = 0;
    iopts->headers       = sl_alloc(vm->arena, sizeof(sl_response_key_value_t) * iopts->header_cap);
    iopts->descriptive_error_pages = opts->descriptive_error_pages;
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
            if(resp->output_len >= resp->output_cap) {
                resp->output_cap *= 2;
                resp->output = sl_realloc(vm->arena, resp->output, sizeof(SLVAL) * resp->output_cap);
            }
            resp->output[resp->output_len++] = sl_to_s(vm, argv[i]);
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
    size_t i, total_size = 0, offset = 0;
    char* output_buffer;
    sl_string_t* str;
    for(i = 0; i < resp->output_len; i++) {
        str = (sl_string_t*)sl_get_ptr(resp->output[i]);
        total_size += str->buff_len;
    }
    output_buffer = sl_alloc_buffer(vm->arena, total_size + 1);
    for(i = 0; i < resp->output_len; i++) {
        str = (sl_string_t*)sl_get_ptr(resp->output[i]);
        memcpy(output_buffer + offset, str->buff, str->buff_len);
        offset += str->buff_len;
    }
    resp->write(vm, output_buffer, total_size);
    resp->output_len = 0;
    return vm->lib.nil;
}

void
sl_response_clear(sl_vm_t* vm)
{
    response(vm)->output_len = 0;
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
    char* h;
    sl_response_internal_opts_t* resp = response(vm);
    (void)self;
    sl_expect(vm, name, vm->lib.String);
    sl_expect(vm, value, vm->lib.String);
    if(resp->header_count >= resp->header_cap) {
        resp->header_cap *= 2;
        resp->headers = sl_realloc(vm->arena, resp->headers, sizeof(sl_response_key_value_t) * resp->header_cap);
    }
    
    h = sl_to_cstr(vm, name);
    if(strchr(h, '\n') || strchr(h, '\r')) {
        return vm->lib._false;
    }
    resp->headers[resp->header_count].name = h;
    
    h = sl_to_cstr(vm, value);
    if(strchr(h, '\n') || strchr(h, '\r')) {
        return vm->lib._false;
    }
    resp->headers[resp->header_count].value = h;
    
    resp->header_count++;
    return vm->lib._true;
}

static SLVAL
response_set_cookie(sl_vm_t* vm, SLVAL self, SLVAL name, SLVAL value)
{
    SLVAL header;
    sl_expect(vm, name, vm->lib.String);
    sl_expect(vm, value, vm->lib.String);
    name = sl_string_url_encode(vm, name);
    value = sl_string_url_encode(vm, value);
    
    header = name;
    header = sl_string_concat(vm, header, sl_make_cstring(vm, "="));
    header = sl_string_concat(vm, header, value);
    
    return response_set_header(vm, self, sl_make_cstring(vm, "Set-Cookie"), header);
}

static SLVAL
response_redirect(sl_vm_t* vm, SLVAL self, SLVAL url)
{
    response(vm)->status = 302;
    response_set_header(vm, self, sl_make_cstring(vm, "Location"), url);
    sl_exit(vm, sl_make_int(vm, 0));
    return self; /* never reached */
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

extern char* sl__error_page_src;

void
sl_render_error_page(sl_vm_t* vm, SLVAL err)
{
    sl_catch_frame_t frame;
    sl_response_internal_opts_t* resp = response(vm);
    resp->status = 500;
    if(resp->descriptive_error_pages) {
        SL_TRY(frame, SL_UNWIND_EXCEPTION, {
            sl_set_global(vm, "err", err);
            sl_do_string(vm, (uint8_t*)sl__error_page_src, strlen(sl__error_page_src), (uint8_t*)"(error-page)");
        }, err, {
            sl_response_write(vm, 
                sl_string_concat(vm,
                    sl_make_cstring(vm, "<h1>Internal Server Error</h1><pre>"),
                    sl_string_concat(vm,
                        sl_string_html_escape(vm, sl_to_s_no_throw(vm, err)),
                        sl_make_cstring(vm, "</pre>"))));
        });
    } else {
        sl_response_write(vm, sl_make_cstring(vm, "<h1>Internal Server Error</h1>"));
    }
}

void
sl_init_response(sl_vm_t* vm)
{
    SLVAL Response = sl_new(vm, vm->lib.Object, 0, NULL);
    sl_vm_store_put(vm, &Response_, Response);
    
    sl_define_singleton_method(vm, Response, "write", -2, sl_response_write);
    sl_define_method(vm, vm->lib.Object, "print", -2, response_write);
    
    sl_define_singleton_method(vm, Response, "flush", 0, sl_response_flush);
    sl_define_singleton_method(vm, Response, "unbuffer", 0, response_unbuffer);
    sl_define_singleton_method(vm, Response, "set_header", 2, response_set_header);
    sl_define_singleton_method(vm, Response, "set_cookie", 2, response_set_cookie);
    sl_define_singleton_method(vm, Response, "redirect", 1, response_redirect);
    sl_define_singleton_method(vm, Response, "status", 0, response_status);
    sl_define_singleton_method(vm, Response, "status=", 1, response_status_set);
    sl_define_singleton_method(vm, Response, "descriptive_error_pages", 0, response_descriptive_error_pages);
    sl_define_singleton_method(vm, Response, "descriptive_error_pages=", 1, response_descriptive_error_pages_set);
    
    sl_class_set_const(vm, vm->lib.Object, "Response", Response);
}
