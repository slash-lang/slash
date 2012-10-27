#include <slash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include "mongoose.h"

#define SERVER_NAME "Slash development server"

static char* PORT = "3000";
static char* LISTEN = "127.0.0.1";
static char* DOCUMENT_ROOT = ".";

static void
usage()
{
    fprintf(stderr, "Usage: slash-server [options]\n\n");
    fprintf(stderr, "    -p, --port     PORT    Port number to listen on. Default: %s\n", PORT);
    fprintf(stderr, "    -l, --listen   IP      IP address to listen on. Default: %s\n", LISTEN);
    fprintf(stderr, "    -r, --root     PATH    Root directory to serve from. Default: %s\n", DOCUMENT_ROOT);
    fprintf(stderr, "    -h, --help             Display these usage instructions\n\n");
    exit(1);
}

static void
error_and_usage(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n\n");
    usage();
}

static void
parse_opts(int argc, char** argv)
{
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if(i + 1 == argc) {
                error_and_usage("Expected port number after %s", argv[i]);
            }
            int p = atoi(argv[++i]);
            if(p < 1 || p > 65535) {
                error_and_usage("Port %d out of bounds.", p);
            }
            PORT = argv[i];
            continue;
        }
        if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--listen") == 0) {
            if(i + 1 == argc) {
                error_and_usage("Expected IP address after %s", argv[i]);
            }
            LISTEN = argv[++i];
            continue;
        }
        if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0) {
            if(i + 1 == argc) {
                error_and_usage("Expected path after %s", argv[i]);
            }
            DOCUMENT_ROOT = argv[++i];
            continue;
        }
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
        }
        error_and_usage("Unknown option %s", argv[i]);
    }
}

static void
quit(int sig)
{
    (void)sig;
    exit(0);
}

static void
msg(const char* fmt, ...)
{
    char buff[200];
    time_t ts = time(NULL);
    struct tm* tptr = localtime(&ts);
    strftime(buff, 200, "[%Y-%m-%d %H:%M:%S] ", tptr);
    printf("%s", buff);
    
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    
    printf("\n");
}

typedef struct {
    sl_vm_t* vm;
    struct mg_request_info* req;
    struct mg_connection* conn;
    int headers_sent;
}
slash_context_t;

static void
setup_request_object(sl_vm_t* vm, struct mg_request_info* req, struct mg_connection* conn)
{
    sl_request_opts_t opts;
    opts.method       = (char*)req->request_method;
    opts.uri          = (char*)req->uri;
    opts.path_info    = NULL;
    opts.query_string = (char*)req->query_string;
    opts.content_type = NULL;
    
    opts.remote_addr  = (char*)sl_alloc_buffer(vm->arena, 20);
    sprintf(opts.remote_addr, "%d.%d.%d.%d",
        (int)((req->remote_ip >> 24) & 255),
        (int)((req->remote_ip >> 16) & 255),
        (int)((req->remote_ip >> 8) & 255),
        (int)((req->remote_ip >> 0) & 255));
    
    opts.header_count = req->num_headers;
    opts.headers = sl_alloc(vm->arena, sizeof(sl_request_key_value_t) * req->num_headers);
    for(int i = 0; i < req->num_headers; i++) {
        opts.headers[i].name = (char*)req->http_headers[i].name;
        opts.headers[i].value = (char*)req->http_headers[i].value;
    }
    
    /* @TODO */
    opts.env_count = 0;
    opts.env = NULL;
    
    size_t buff_len = 0;
    size_t buff_cap = 4096;
    void* buff = sl_alloc_buffer(vm->arena, buff_cap);
    while(1) {
        buff_len += mg_read(conn, buff + buff_len, buff_cap - buff_len);
        if(buff_len < buff_cap) {
            break;
        }
        buff = sl_realloc(vm->arena, buff, buff_cap *= 2);
    }
    opts.post_data = buff;
    opts.post_length = buff_len;
}

static void
flush_headers(slash_context_t* ctx)
{
    if(!ctx->headers_sent) {
        mg_printf(ctx->conn, "HTTP/%s %d\r\n", ctx->req->http_version, sl_response_get_status(ctx->vm));
        size_t header_count;
        sl_response_key_value_t* headers = sl_response_get_headers(ctx->vm, &header_count);
        int sent_content_type = 0;
        for(size_t i = 0; i < header_count; i++) {
            if(strcmp(headers[i].name, "Content-Type") == 0) {
                sent_content_type = 1;
            }
            mg_printf(ctx->conn, "%s: %s\r\n", headers[i].name, headers[i].value);
        }
        if(!sent_content_type) {
            mg_printf(ctx->conn, "Content-Type: text/html; charset=utf-8\r\n");
        }
        mg_printf(ctx->conn, "\r\n");
        ctx->headers_sent = 1;
    }
}

static void
slash_output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    flush_headers(ctx);
    mg_printf(ctx->conn, "%.*s", (int)len, buff);
}

static void
setup_response_object(sl_vm_t* vm)
{
    sl_response_opts_t opts;
    opts.write                   = slash_output;
    opts.buffered                = 1;
    opts.descriptive_error_pages = 1;
    sl_response_set_opts(vm, &opts);
}

static void
run_slash_script(char* path, struct mg_request_info* req, struct mg_connection* conn, void* stack_top)
{
    slash_context_t ctx = {
        .vm = sl_init(),
        .headers_sent = 0,
        .conn = conn,
        .req = req
    };
    
    sl_vm_t* vm = ctx.vm;
    
    ctx.vm->data = &ctx;
    sl_gc_set_stack_top(vm->arena, stack_top);
    
    vm->cwd = sl_alloc_buffer(vm->arena, strlen(path) + 1);
    strcpy(vm->cwd, path);
    char* last_slash = strrchr(vm->cwd, '/');
    if(last_slash) {
        *last_slash = 0;
    }
    
    setup_request_object(vm, req, conn);
    setup_response_object(vm);
    
    sl_catch_frame_t exit_frame, exception_frame;
    SLVAL error;
    
    SL_TRY(exit_frame, SL_UNWIND_ALL, {
        SL_TRY(exception_frame, SL_UNWIND_EXCEPTION, {
            sl_do_file(vm, path);
        }, error, {
            sl_response_clear(vm);
            sl_render_error_page(vm, error);
        });
    }, error, {});
    
    sl_response_flush(vm);
    sl_free_gc_arena(vm->arena);
}

static void*
on_event(enum mg_event event, struct mg_connection* conn)
{
    struct mg_request_info* req = mg_get_request_info(conn);
    if(event == MG_NEW_REQUEST) {
        msg("%s %s", req->request_method, req->uri);
        size_t len = strlen(req->uri);
        /* TODO: work with path info */
        if(strcmp(req->uri + len - 3, ".sl") != 0) {
            return NULL;
        }
        char* path = malloc(strlen(req->uri) + strlen(DOCUMENT_ROOT) + 1);
        strcpy(path, DOCUMENT_ROOT);
        strcat(path, req->uri);
        FILE* f = fopen(path, "r");
        if(f) {
            fclose(f);
            run_slash_script(path, req, conn, &req);
            free(path);
            return "";
        }
        free(path);
        return NULL;
    } else {
        return NULL;
    }
}

int
main(int argc, char** argv)
{
    sl_static_init();
    
    parse_opts(argc, argv);
    
    signal(SIGTERM, quit);
    signal(SIGINT, quit);
    
    fprintf(stderr, "%s - starting on %s:%s\n", SERVER_NAME, LISTEN, PORT);
    
    const char* mongoose_opts[] = {
        "listening_ports",  PORT,
        "index_files",      "index.html,index.htm,index.sl",
        "document_root",    DOCUMENT_ROOT
    };
    
    struct mg_context *ctx = mg_start(on_event, NULL, mongoose_opts);
    while(1) {
        getchar();
    }
    exit(0);
}
