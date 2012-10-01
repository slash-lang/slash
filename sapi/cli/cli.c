#include <slash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t* src;
    size_t len;
    uint8_t* filename;
}
cli_state_t;

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    (void)vm;
    fwrite(buff, len, 1, stdout);
}

static void
run(sl_vm_t* vm, void* state)
{
    size_t token_count;
    sl_token_t* tokens;
    sl_node_base_t* ast;
    sl_vm_section_t* section;
    cli_state_t* st = state;
    sl_vm_exec_ctx_t ctx;
    tokens = sl_lex(vm, st->filename, st->src, st->len, &token_count);
    ast = sl_parse(vm, tokens, token_count, st->filename);
    section = sl_compile(vm, ast, st->filename);
    ctx.vm = vm;
    ctx.section = section;
    ctx.registers = sl_alloc(vm->arena, sizeof(SLVAL) * section->max_registers);
    ctx.self = vm->lib.Object;
    ctx.parent = NULL;
    sl_vm_exec(&ctx);
}

static void
on_error(sl_vm_t* vm, void* state, SLVAL error)
{
    SLVAL error_str = sl_to_s(vm, error);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(error_str);
    (void)state;
    fwrite(str->buff, str->buff_len, 1, stderr);
    puts("\n");
    exit(1);
}

static uint8_t*
read_all(sl_vm_t* vm, FILE* f, size_t* length)
{
    size_t len = 0;
    size_t cap = 4096;
    uint8_t* src = sl_alloc_buffer(vm->arena, cap);
    while(!feof(f)) {
        if(len + 4096 > cap) {
            cap *= 2;
            src = sl_realloc(vm->arena, src, cap);
        }
        len += fread(src + len, 1, 4096, f);
    }
    *length = len;
    return src;
}

static void
setup_request_response(sl_vm_t* vm)
{
    sl_request_opts_t req;
    sl_response_opts_t res;
    
    req.method       = "";
    req.uri          = "";
    req.path_info    = "";
    req.query_string = "";
    req.remote_addr  = "";
    req.content_type = "";
    req.header_count = 0;
    req.env_count    = 0;
    req.post_length  = 0;
    sl_request_set_opts(vm, &req);
    
    res.buffered                = 0;
    res.write                   = output;
    res.descriptive_error_pages = 0;
    sl_response_set_opts(vm, &res);
}

int
main(int argc, char** argv)
{
    cli_state_t state;
    FILE* f;
    sl_vm_t* vm;
    sl_catch_frame_t exit_frame;
    SLVAL err;
    int exit_code;
    sl_static_init();
    vm = sl_init();
    sl_gc_set_stack_top(vm->arena, &argc);
    setup_request_response(vm);
    if(argc > 1) {
        state.filename = (uint8_t*)argv[1];
        f = fopen((char*)state.filename, "rb");
        if(!f) {
            fprintf(stderr, "Can't open '%s' for reading\n", state.filename);
            exit(1);
        }
    } else {    
        state.filename = (uint8_t*)"(stdin)";
        f = stdin;
    }
    state.src = read_all(vm, f, &state.len);
    SL_TRY(exit_frame, SL_UNWIND_ALL, {
        run(vm, &state);
    }, err, {
        if(exit_frame.type == SL_UNWIND_EXCEPTION) {
            on_error(vm, &state, err);
        }
        if(exit_frame.type == SL_UNWIND_EXIT) {
            exit_code = sl_get_int(err);
            sl_free_gc_arena(vm->arena);
            exit(exit_code);
        }
    });    
    sl_free_gc_arena(vm->arena);
    return 0;
}
