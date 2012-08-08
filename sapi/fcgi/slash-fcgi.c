#include "slash.h"
#include "lib/request.h"
#include "lib/response.h"
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

#define USE_PTHREADS

typedef struct slash_context {
    FCGX_Request request;
    sl_vm_t* vm;
    int sent_headers;
    #ifdef USE_PTHREADS
        pthread_t thread;
    #endif
}
slash_context_t;

static void*
handle_request_thread_start(void* arg);

static void
handle_request(slash_context_t* ctx, void* dummy);

static char*
LISTEN = ":9000";

static int
BACKLOG = 16;

static int
DAEMONIZE = 0;

static void
usage()
{
    fprintf(stderr, "Usage: slash-fcgi [options]\n"
                    "\n"
                    "    -l, --listen  :PORT                Port number to listen on. Default: 9000\n"
                    "    -l, --listen  IP:PORT              IP address and port number to listen on\n"
                    "    -l, --listen  /path/to/socket      Unix socket to listen on\n"
                    "\n"
                    "    -b, --backlog BACKLOG              Connection backlog. Default: 16\n"
                    "\n"
                    "    -d, --daemonize                    Start slash-fcgi as a daemon. Default: off\n"
                    "\n"
                    "    -h, --help                         Display these usage instructions\n"
                    "\n");
    exit(0);
}

static void
parse_opts(int argc, char** argv)
{
    int i;
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--listen") == 0) {
            if(++i >= argc) {
                fprintf(stderr, "Expected :PORT, IP:PORT or file path after %s\n", argv[i - 1]);
                exit(1);
            }
            LISTEN = argv[i];
            continue;
        }
        if(strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--backlog") == 0) {
            if(++i >= argc) {
                fprintf(stderr, "Expected BACKLOG after %s\n", argv[i - 1]);
                exit(1);
            }
            BACKLOG = atoi(argv[i]);
            continue;
        }
        if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemonize") == 0) {
            DAEMONIZE = 1;
            continue;
        }
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        exit(1);
    }
}

void
stop_server()
{
    fprintf(stderr, "Goodbye.\n");
    exit(0);
}

int
main(int argc, char** argv)
{
    int sock;
    slash_context_t* ctx;
    #ifdef USE_PTHREADS
        pthread_t thread;
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    #endif
    
    parse_opts(argc, argv);
    FCGX_Init();
    sl_static_init();
    
    sock = FCGX_OpenSocket(LISTEN, BACKLOG);
    while(1) {
        ctx = malloc(sizeof(slash_context_t));
        FCGX_InitRequest(&ctx->request, sock, 0);
        if(FCGX_Accept_r(&ctx->request) != 0) {
            exit(1);
        }
        #ifdef USE_PTHREADS
            pthread_create(&thread, &thread_attr, handle_request_thread_start, ctx);
        #else
            handle_request_thread_start(ctx);
        #endif
    }
}

static void*
handle_request_thread_start(void* arg)
{
    int dummy;
    handle_request(arg, &dummy);
    return NULL;
}

static void
setup_request_object(sl_vm_t* vm, FCGX_Request* request, char** script_filename)
{
    sl_request_opts_t opts;
    size_t i, j, env_i = 0, header_i = 0;
    char* value;
    opts.method = "GET";
    opts.uri = "";
    opts.path_info = NULL;
    opts.query_string = NULL;
    opts.remote_addr = "";
    for(i = 0; request->envp[i]; i++) {
        env_i++;
    }
    opts.env = sl_alloc(vm->arena, sizeof(sl_request_key_value_t) * env_i);
    opts.headers = sl_alloc(vm->arena, sizeof(sl_request_key_value_t) * env_i);
    env_i = 0;
    for(i = 0; request->envp[i]; i++) {
        value = strchr(request->envp[i], '=');
        if(!value) {
            continue;
        }
        *value++ = 0;
        opts.env[env_i].name = request->envp[i];
        opts.env[env_i].value = value;
        env_i++;
        if(memcmp(request->envp[i], "HTTP_", 5) == 0) {
            opts.headers[header_i].name = sl_alloc(vm->arena, strlen(request->envp[i]));
            for(j = 0; request->envp[i][j + 5]; j++) {
                if(request->envp[i][j + 5] == '_') {
                    opts.headers[header_i].name[j] = '-';
                } else {
                    if(request->envp[i][j + 5 - 1] == '_') {
                        opts.headers[header_i].name[j] = request->envp[i][j + 5];
                    } else {
                        opts.headers[header_i].name[j] = tolower(request->envp[i][j + 5]);
                    }
                }
            }
            opts.headers[header_i].value = value;
            header_i++;
        }
        if(strcmp(request->envp[i], "REQUEST_METHOD") == 0)  { opts.method       = value; }
        if(strcmp(request->envp[i], "REQUEST_URI") == 0)     { opts.uri          = value; }
        if(strcmp(request->envp[i], "PATH_INFO") == 0)       { opts.path_info    = value; }
        if(strcmp(request->envp[i], "QUERY_STRING") == 0)    { opts.query_string = value; }
        if(strcmp(request->envp[i], "REMOTE_ADDR") == 0)     { opts.remote_addr  = value; }
        if(strcmp(request->envp[i], "CONTENT_TYPE") == 0)    { opts.content_type = value; }
        if(strcmp(request->envp[i], "SCRIPT_FILENAME") == 0) { *script_filename  = value; }
        /* @TODO: lol: */
        if(strcmp(request->envp[i], "QUERY_STRING") == 0 && strcmp(value, "exit") == 0) {
            exit(0);
        }
    }
    opts.header_count = header_i;
    opts.env_count    = env_i;
    opts.post_data    = "";
    opts.post_length  = 0;
    sl_request_set_opts(vm, &opts);
}

static void
flush_headers(slash_context_t* ctx)
{
    sl_response_key_value_t* headers;
    size_t header_count, i;
    int sent_content_type = 0;
    if(ctx->sent_headers) {
        return;
    }
    ctx->sent_headers = 1;
    headers = sl_response_get_headers(ctx->vm, &header_count);
    FCGX_FPrintF(ctx->request.out, "Status: %d\r\n", sl_response_get_status(ctx->vm));
    for(i = 0; i < header_count; i++) {
        /* @TODO: do something not non-standard */
        if(strcasecmp(headers[i].name, "Content-Type") == 0) {
            sent_content_type = 1;
        }
        FCGX_FPrintF(ctx->request.out, "%s: %s\r\n", headers[i].name, headers[i].value);
    }
    if(!sent_content_type) {
        FCGX_PutS("Content-Type: text/html; charset=utf-8\r\n", ctx->request.out);
    }
    FCGX_PutS("\r\n", ctx->request.out);
}

static void
output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    flush_headers(ctx);
    FCGX_PutStr(buff, (int)len, ctx->request.out);
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

static void
handle_request(slash_context_t* ctx, void* dummy)
{
    sl_catch_frame_t frame;
    SLVAL err;
    char* script_filename = "";
    sl_vm_t* vm = sl_init();
    pthread_t me = ctx->thread;
    ctx->vm = vm;
    ctx->vm->data = ctx;
    ctx->sent_headers = 0;
    sl_gc_set_stack_top(ctx->vm->arena, dummy);
    
    setup_request_object(ctx->vm, &ctx->request, &script_filename);
    setup_response_object(ctx->vm);
    SL_TRY(frame, SL_UNWIND_ALL, {
        sl_do_file(ctx->vm, (uint8_t*)script_filename);
    }, err, {
        if(frame.type == SL_UNWIND_EXCEPTION) {
            sl_response_clear(ctx->vm);
            sl_render_error_page(ctx->vm, err);
        }
    });
    flush_headers(ctx);
    sl_response_flush(ctx->vm);
    
    sl_free_gc_arena(ctx->vm->arena);
    FCGX_PutS("Content-Type: text/html\r\n\r\nHello world", ctx->request.out);
    FCGX_Finish_r(&ctx->request);
    FCGX_Free(&ctx->request, 0);
    free(ctx);
    pthread_cancel(me);
}
