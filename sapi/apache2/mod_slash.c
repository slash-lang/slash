#include <gc.h>
#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include <util_script.h>
#include "slash.h"
#include "platform.h"
#include "lib/request.h"
#include "lib/response.h"

typedef struct {
    sl_vm_t* vm;
    request_rec* r;
    int headers_sent;
}
slash_context_t;

struct iter_args {
    sl_request_key_value_t* kvs;
    size_t count;
    size_t capacity;
};

static void
flush_headers(slash_context_t* ctx)
{
    sl_response_key_value_t* headers;
    size_t header_count, i;
    if(!ctx->headers_sent) {
        ctx->r->status = sl_response_get_status(ctx->vm);
        ctx->r->status_line = ap_get_status_line(ctx->r->status);
        headers = sl_response_get_headers(ctx->vm, &header_count);
        for(i = 0; i < header_count; i++) {
            apr_table_add(ctx->r->headers_out, headers[i].name, headers[i].value);
        }
        ctx->headers_sent = 1;
    }
}

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    flush_headers(ctx);
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
read_post_data(sl_request_opts_t* opts, request_rec* r)
{
    apr_bucket_brigade* brigade = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    size_t len = 1024;
    opts->post_length = 0;
    opts->post_data = NULL;
    while(ap_get_brigade(r->input_filters, brigade, AP_MODE_READBYTES, APR_BLOCK_READ, len) == APR_SUCCESS) {
        opts->post_data = GC_REALLOC(opts->post_data, opts->post_length + len);
        apr_brigade_flatten(brigade, opts->post_data + opts->post_length, &len);
        apr_brigade_cleanup(brigade);
        opts->post_length += len;
        if(!len) {
            break;
        }
        len = 1024;
    }
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
    opts.content_type = (char*)apr_table_get(r->headers_in, "content-type");
    
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
    
    read_post_data(&opts, r);
    
    sl_request_set_opts(vm, &opts);
}

static void
setup_response_object(sl_vm_t* vm)
{
    sl_response_opts_t opts;
    opts.write                   = output;
    opts.buffered                = 1;
    opts.descriptive_error_pages = 1;
    sl_response_set_opts(vm, &opts);
}

static int
run_slash_script(request_rec* r)
{
    sl_vm_t* vm;
    slash_context_t ctx;
    sl_catch_frame_t exit_frame, exception_frame;
    SLVAL error;
    sl_static_init();
    vm = sl_init();
    SL_TRY(exit_frame, SL_UNWIND_ALL, {
        SL_TRY(exception_frame, SL_UNWIND_EXCEPTION, {
            ctx.headers_sent = 0;
            ctx.vm = vm;
            ctx.r = r;
            vm->data = &ctx;
            setup_request_object(vm, r);
            setup_response_object(vm);
            ap_set_content_type(r, "text/html; charset=utf-8");
            sl_do_file(vm, (uint8_t*)r->canonical_filename);
        }, error, {
            sl_response_clear(vm);
            sl_render_error_page(vm, error);
        });
    }, error, {});
    flush_headers(&ctx);
    sl_response_flush(vm);
    return OK;
}

static int
slash_handler(request_rec* r)
{
    if(!r->handler || strcmp(r->handler, "slash") != 0) {
        return DECLINED;
    }
    if(!sl_file_exists(r->canonical_filename)) {
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
