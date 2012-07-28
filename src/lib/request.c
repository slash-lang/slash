#include <gc.h>
#include "lib/request.h"
#include "lib/dict.h"

typedef struct {
    SLVAL method;
    SLVAL uri;
    SLVAL remote_addr;
    SLVAL headers;
    SLVAL get;
    SLVAL post;
    SLVAL params;
}
sl_request_internal_opts_t;

static int Request_;
static int Request_opts;

sl_request_internal_opts_t*
request(sl_vm_t* vm)
{
    return (sl_request_internal_opts_t*)sl_get_ptr(sl_vm_store_get(vm, &Request_opts));
}

void
sl_request_set_opts(sl_vm_t* vm, sl_request_opts_t* opts)
{
    size_t i;
    SLVAL n, v;
    sl_request_internal_opts_t* req = GC_MALLOC(sizeof(sl_request_internal_opts_t));
    req->method      = sl_make_cstring(vm, opts->method);
    req->uri         = sl_make_cstring(vm, opts->uri);
    req->remote_addr = sl_make_cstring(vm, opts->remote_addr);
    req->headers     = sl_make_dict(vm, 0, NULL);
    req->get         = sl_make_dict(vm, 0, NULL);
    req->post        = sl_make_dict(vm, 0, NULL);
    req->params      = sl_make_dict(vm, 0, NULL);
    for(i = 0; i < opts->header_count; i++) {
        n = sl_make_cstring(vm, opts->headers[i].name);
        v = sl_make_cstring(vm, opts->headers[i].value);
        sl_dict_set(vm, req->headers, n, v);
    }
    for(i = 0; i < opts->get_count; i++) {
        n = sl_make_cstring(vm, opts->get_params[i].name);
        v = sl_make_cstring(vm, opts->get_params[i].value);
        sl_dict_set(vm, req->get, n, v);
        sl_dict_set(vm, req->params, n, v);
    }
    for(i = 0; i < opts->post_count; i++) {
        n = sl_make_cstring(vm, opts->post_params[i].name);
        v = sl_make_cstring(vm, opts->post_params[i].value);
        sl_dict_set(vm, req->post, n, v);
        sl_dict_set(vm, req->params, n, v);
    }
    sl_vm_store_put(vm, &Request_opts, sl_make_ptr((sl_object_t*)req));
}

static SLVAL
request_get(sl_vm_t* vm)
{
    return request(vm)->get;
}

static SLVAL
request_post(sl_vm_t* vm)
{
    return request(vm)->post;
}

static SLVAL
request_headers(sl_vm_t* vm)
{
    return request(vm)->headers;
}

static SLVAL
request_index(sl_vm_t* vm, SLVAL self, SLVAL name)
{
    (void)self;
    return sl_dict_get(vm, request(vm)->params, name);
}

static SLVAL
request_method(sl_vm_t* vm)
{
    return request(vm)->method;
}

static SLVAL
request_uri(sl_vm_t* vm)
{
    return request(vm)->uri;
}

static SLVAL
request_remote_addr(sl_vm_t* vm)
{
    return request(vm)->remote_addr;
}

void
sl_init_request(sl_vm_t* vm)
{
    SLVAL Request = sl_new(vm, vm->lib.Object, 0, NULL);
    sl_vm_store_put(vm, &Request_, Request);
    sl_define_singleton_method(vm, Request, "get", 0, request_get);
    sl_define_singleton_method(vm, Request, "post", 0, request_post);
    sl_define_singleton_method(vm, Request, "headers", 0, request_headers);
    sl_define_singleton_method(vm, Request, "method", 0, request_method);
    sl_define_singleton_method(vm, Request, "uri", 0, request_uri);
    sl_define_singleton_method(vm, Request, "remote_addr", 0, request_remote_addr);
    sl_define_singleton_method(vm, Request, "[]", 1, request_index);
    
    sl_class_set_const(vm, vm->lib.Object, "Request", Request);
}
