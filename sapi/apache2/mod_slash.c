#include <gc.h>
#include <apache2/httpd.h>
#include <apache2/http_protocol.h>
#include <apache2/http_config.h>
#include <apache2/util_script.h>
#include "slash.h"
#include "lib/request.h"

typedef struct {
    sl_vm_t* vm;
    request_rec* r;
}
slash_context_t;

struct iter_args {
    sl_request_key_value_t* kvs;
    size_t count;
    size_t capacity;
};

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    ap_rwrite(buff, len, ctx->r);
}

static int
iterate_apr_table(void* rec, const char* name, const char* value)
{
    struct iter_args* ia = rec;
    if(ia->count == ia->capacity) {
        ia->capacity *= 2;
        ia->kvs = GC_REALLOC(ia->kvs, sizeof(sl_request_key_value_t) * ia->capacity);
    }
    ia->kvs[ia->count].name = (char*)name;
    ia->kvs[ia->count].value = (char*)value;
    ia->count++;
    return 1;
}

static void
setup_request_object(sl_vm_t* vm, request_rec* r)
{
    struct iter_args ia;
    
    sl_request_opts_t opts;
    opts.method       = (char*)r->method;
    opts.uri          = r->uri;
    opts.path_info    = r->path_info;
    opts.query_string = r->args;
    opts.remote_addr  = r->connection->remote_ip;
    
    ia.count = 0;
    ia.capacity = 4;
    ia.kvs = GC_MALLOC(sizeof(sl_request_key_value_t) * ia.capacity);
    apr_table_do(iterate_apr_table, &ia, r->headers_in, NULL);
    opts.header_count = ia.count;
    opts.headers = ia.kvs;
    
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);
    ia.count = 0;
    ia.capacity = 4;
    ia.kvs = GC_MALLOC(sizeof(sl_request_key_value_t) * ia.capacity);
    apr_table_do(iterate_apr_table, &ia, r->subprocess_env, NULL);
    opts.env_count = ia.count;
    opts.env = ia.kvs;
    
    opts.post_count   = 0;
    opts.post_params  = NULL;
    sl_request_set_opts(vm, &opts);
}

static int
run_slash_script(request_rec* r)
{
    sl_vm_t* vm;
    slash_context_t ctx;
    sl_catch_frame_t frame;
    SLVAL error;
    sl_static_init();
    vm = sl_init();
    SL_TRY(frame, {
        ctx.vm = vm;
        ctx.r = r;
        vm->data = &ctx;
        vm->output = output;
        setup_request_object(vm, r);
        ap_set_content_type(r, "text/html; charset=utf-8");
        sl_do_file(vm, (uint8_t*)r->canonical_filename);
    }, error, {    
        ap_set_content_type(r, "text/html; charset=utf-8");
        error = sl_string_html_escape(vm, sl_to_s(vm, error));
        ap_rputs(sl_to_cstr(vm, error), r);
    });
    return OK;
}

static int
slash_handler(request_rec* r)
{
    if(!r->handler || strcmp(r->handler, "slash") != 0) {
        return DECLINED;
    }
    return run_slash_script(r);
}

static void
register_hooks(apr_pool_t* pool)
{
    (void)pool;
    ap_hook_handler(slash_handler, NULL, NULL, APR_HOOK_MIDDLE);
    /*sl_static_init();*/
}

module AP_MODULE_DECLARE_DATA slash_module = {
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    register_hooks
};
