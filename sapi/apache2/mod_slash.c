#include <apache2/httpd.h>
#include <apache2/http_protocol.h>
#include <apache2/http_config.h>
#include "slash.h"

typedef struct {
    sl_vm_t* vm;
    request_rec* r;
}
slash_context_t;

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    ap_rwrite(buff, len, ctx->r);
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
        ap_set_content_type(r, "text/html; charset=utf-8");
        sl_do_file(vm, (uint8_t*)r->canonical_filename);
    }, error, {    
        ap_set_content_type(r, "text/html; charset=utf-8");
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
