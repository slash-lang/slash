#include <slash.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "readline.h"

#ifdef SL_HAS_UNISTD
    #include <unistd.h>
#endif

#ifdef __APPLE__
    char*** _NSGetEnviron();
    #define environ (*(_NSGetEnviron()))
#endif

static bool
opt_interactive;

static FILE*
opt_source_file;

static char*
opt_source_file_name;

static void
setup_vm_request(sl_vm_t* vm)
{
    sl_request_opts_t req;
    req.method          = "";
    req.uri             = "";
    req.path_info       = "";
    req.query_string    = "";
    req.remote_addr     = "";
    req.content_type    = "";

    req.header_count    = 0;
    req.post_length     = 0;

    req.env_count = 0;
    for(char** env = environ; *env; env++) {
        req.env_count++;
    }

    sl_request_key_value_t env[req.env_count];
    for(size_t i = 0; i < req.env_count; i++) {
        char* e = sl_alloc(vm->arena, strlen(environ[i]) + 1);
        strcpy(e, environ[i]);
        char* v = strchr(e, '=');
        if(!v) {
            continue;
        }
        *v = 0;
        env[i].name = e;
        env[i].value = v + 1;
    }

    req.env = env;
    sl_request_set_opts(vm, &req);
}

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    fwrite(buff, len, 1, stdout);
    fflush(stdout);
}

static void
setup_vm_response(sl_vm_t* vm)
{
    sl_response_opts_t res;
    res.descriptive_error_pages = 0;
    res.buffered = 0;
    res.write = output;
    sl_response_set_opts(vm, &res);
}

static sl_vm_t*
setup_vm(void* stack_top)
{
    sl_static_init();
    sl_vm_t* vm = sl_init();
    sl_gc_set_stack_top(vm->arena, stack_top);
    setup_vm_request(vm);
    setup_vm_response(vm);
    return vm;
}

static void
shutdown_vm(sl_vm_t* vm, int exit_code)
{
    sl_free_gc_arena(vm->arena);
    exit(exit_code);
}

static void
print_help(char* program_name)
{
    fprintf(stderr, "Usage: %s [options] [--] [source-file] [arguments...]\n", program_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "   -I <path>       Adds <path> to the list of paths searched when requiring files\n");
    fprintf(stderr, "   -i              Starts an interactive REPL session\n");
    fprintf(stderr, "   -h              Prints this help\n");
}

static void
process_arguments(sl_vm_t* vm, int argc, char** argv)
{
    int i = 1;
    for(; i < argc; i++) {
        if(strcmp(argv[i], "-I") == 0) {
            if(i + 1 == argc) {
                fprintf(stderr, "Expected <path> after -I\n");
                shutdown_vm(vm, 1);
            }
            sl_require_path_add(vm, argv[++i]);
            continue;
        }
        if(strcmp(argv[i], "-i") == 0) {
            opt_interactive = true;
            continue;
        }
        if(strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            shutdown_vm(vm, 0);
        }
        if(strcmp(argv[i], "--") == 0) {
            i++;
            break;
        }
        if(argv[i][0] == '-') {
            fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            shutdown_vm(vm, 1);
        }
        break;
    }
    if(!opt_interactive) {
        if(i == argc || strcmp(argv[i], "-") == 0) {
            opt_source_file = stdin;
            opt_source_file_name = "(stdin)";
        } else {
            opt_source_file = fopen(argv[i], "rb");
            opt_source_file_name = argv[i];
            if(!opt_source_file) {
                perror("Could not open source file");
                shutdown_vm(vm, 1);
            }
        }
        i++;
    }
    SLVAL vargv = sl_make_array(vm, 0, NULL);
    for(; i < argc; i++) {
        SLVAL varg = sl_make_cstring(vm, argv[i]);
        sl_array_push(vm, vargv, 1, &varg);
    }
    sl_class_set_const(vm, vm->lib.Object, "ARGV", vargv);
}

static void
handle_exception_and_exit(sl_vm_t* vm, SLVAL error)
{
    SLVAL error_str = sl_to_s_no_throw(vm, error);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(error_str);
    fwrite(str->buff, str->buff_len, 1, stderr);
    fprintf(stderr, "\n\n");
    shutdown_vm(vm, 1);
}

static void
run_repl(sl_vm_t* vm)
{
    printf("Interactive Slash\n");
    cli_setup_readline();
    while(true) {
        char* line = cli_readline(">> ");
        if(!line) {
            printf("\n");
            shutdown_vm(vm, 0);
        }
        sl_vm_frame_t frame;
        SLVAL exception;
        SL_TRY(frame, SL_UNWIND_EXCEPTION, {
            SLVAL result = sl_do_string(vm, (uint8_t*)line, strlen(line), "(repl)", 1);
            result = sl_inspect(vm, result);
            printf("=> ");
            sl_string_t* str = (sl_string_t*)sl_get_ptr(result);
            fwrite(str->buff, str->buff_len, 1, stdout);
            printf("\n");
        }, exception, {
            SLVAL exception_str = sl_to_s_no_throw(vm, exception);
            sl_string_t* str = (sl_string_t*)sl_get_ptr(exception_str);
            fwrite(str->buff, str->buff_len, 1, stdout);
            printf("\n");
        });
    }
}

static void
run_script(sl_vm_t* vm)
{
    size_t source_cap = 4096;
    size_t source_len = 0;
    uint8_t* source = malloc(source_cap);
    while(!feof(opt_source_file) && !ferror(opt_source_file)) {
        if(source_len + 4096 > source_cap) {
            source_cap += 4096;
            source = realloc(source, source_cap);
        }
        source_len += fread(source, 1, 4096, opt_source_file);
    }
    if(ferror(opt_source_file)) {
        perror("Error while reading source file");
    }
    sl_do_string(vm, source, source_len, opt_source_file_name, 0);
}

int
main(int argc, char** argv)
{
    sl_vm_t* vm = setup_vm(&argc);

    #ifdef SL_HAS_UNISTD
        if(isatty(0)) {
            opt_interactive = true;
        }
    #endif

    process_arguments(vm, argc, argv);

    sl_vm_frame_t frame;
    SLVAL exception;
    SL_TRY(frame, SL_UNWIND_ALL, {
        if(opt_interactive) {
            run_repl(vm);
        } else {
            run_script(vm);
        }
    }, exception, {
        if(frame.as.handler_frame.unwind_type == SL_UNWIND_EXIT) {
            shutdown_vm(vm, 0);
        }
        if(frame.as.handler_frame.unwind_type == SL_UNWIND_EXCEPTION) {
            handle_exception_and_exit(vm, exception);
        }
    });
}
