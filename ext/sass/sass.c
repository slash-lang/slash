#include <sass_interface.h>
#include <slash.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static SLVAL
sl_sass_compile(sl_vm_t* vm, SLVAL klass, SLVAL source)
{
    struct sass_context* sass_ctx = sass_new_context();
    sass_ctx->source_string = sl_to_cstr(vm, source);
    sass_ctx->options.output_style = SASS_STYLE_NESTED;
    sass_ctx->options.include_paths = "";
    sass_ctx->options.image_path = "";
    sass_compile(sass_ctx);
    if(sass_ctx->error_status) {
        char* err_cpy;
        if(sass_ctx->error_message) {
            err_cpy = sl_alloc_buffer(vm->arena, strlen(sass_ctx->error_message) + 1);
            strcpy(err_cpy, sass_ctx->error_message);
        } else {
            err_cpy = "SASS compilation error";
        }
        sass_free_context(sass_ctx);
        sl_throw_message2(vm, vm->lib.SyntaxError, err_cpy);
    }
    SLVAL result = sl_make_cstring(vm, sass_ctx->output_string);
    sass_free_context(sass_ctx);
    return result;
    
    (void)klass;
}

void
sl_init_ext_sass(sl_vm_t* vm)
{
    SLVAL SASS = sl_define_class(vm, "SASS", vm->lib.Object);
    sl_define_singleton_method(vm, SASS, "compile", 1, sl_sass_compile);
}
