#include <slash.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <fcgiapp.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <signal.h>

#include "api.h"
#include "http_status.h"

extern char** environ;

typedef struct {
    sl_request_key_value_t* kvs;
    size_t count;
    size_t capacity;
    sl_vm_t* vm;
} sl_request_key_value_list_t;

typedef struct {
    sl_request_key_value_list_t* environment;
    sl_request_key_value_list_t* http_headers;

    char* real_uri;
    char* real_path_info;
    char* real_canonical_filename;
    char* real_canonical_dir;

    sl_request_key_value_t* script_name;
    sl_request_key_value_t* path_translated;
    sl_request_key_value_t* path_info;
    sl_request_key_value_t* script_filename;
    sl_request_key_value_t* content_length;
    sl_request_key_value_t* content_type;
    sl_request_key_value_t* request_uri;
    sl_request_key_value_t* query_string;
    sl_request_key_value_t* remote_addr;
    sl_request_key_value_t* request_method;
} slash_request_info_t;

#ifdef SL_TEST
static void register_cgi_test_utils(sl_vm_t* vm);
#endif

typedef struct {
    sl_vm_t* vm;
    slash_api_base_t* api;
    int headers_sent;
    slash_request_info_t info;
}
slash_context_t;

typedef struct {
    size_t incpaths_capacity;
    size_t incpaths_count;
    char** incpaths;
    char* fcgi_bind;
    int fcgi_backlog;
    int fcgi_num_childs;
    int fcgi_num_requests;

    char* arg_script_filename;
    size_t arg_script_options_capacity;
    size_t arg_script_options_count;
    char** arg_script_options;
} cgi_options;

static sl_request_key_value_list_t*
sl_request_key_value_list_new(sl_vm_t* vm, size_t capacity)
{
   sl_request_key_value_list_t* list =
    sl_alloc(vm->arena, sizeof(sl_request_key_value_list_t));

   if(list) {
       list->kvs = sl_alloc(vm->arena,
            sizeof(sl_request_key_value_t) * capacity);
       list->count = 0;
       list->capacity = capacity;
       list->vm = vm;
   }

   return list;
} 

static sl_request_key_value_t*
sl_request_key_value_list_add(sl_request_key_value_list_t* list,
                                    char* name, char* value)
{
    if(list->count == list->capacity) {
        list->capacity *= 2;
        list->kvs = sl_realloc(list->vm->arena,
            list->kvs,
            sizeof(sl_request_key_value_t) * list->capacity);
    }

    list->kvs[list->count].name = name;
    list->kvs[list->count].value = value;

    return &list->kvs[list->count++];
}


static size_t
get_hashbang_length(uint8_t* src, size_t len)
{
    size_t length = 0;

    if(len >= 3 && 
        src[0] == '#' &&
        src[1] == '!' &&
        src[2] == '/' ) {
        for(size_t i = 3; i < len && length == 0; ++i) {
            /*
             * Possible newlines:
             * \r\n (Win)
             * x\n  (Unix)
             * \rx  (Old MacOS)
             */
            if(src[i] == '\r' || src[i] == '\n') {
                if((i + 1) < len && src[i] == '\r' && src[i + 1] == '\n') {
                    ++i;
                }

                if((i + 1) <= len) {
                    length = i + 1;
                }
                break;
            }
        }
    }

    return length;
}

static SLVAL
sl_do_file_hashbang(sl_vm_t* vm, char* filename, int skip_hashbang)
{
    FILE* fh;
    uint8_t* src;
    long file_size;
    size_t real_start = 0;

    if(!skip_hashbang) {
        return sl_do_file(vm, filename);
    }

    filename = sl_realpath(vm, filename);
    fh = fopen(filename, "rb");

    if(!fh) {
        sl_error(vm, vm->lib.Error,
            "Could not load %Qs - %s", filename, strerror(errno));
    }

    fseek(fh, 0, SEEK_END);
    file_size = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    if(file_size < 0) {
        sl_error(vm, vm->lib.Error,
            "Could not load %Qs - %s (count not get filesize)",
            filename, strerror(errno));
    }

    src = sl_alloc(vm->arena, file_size);
    if(file_size && !fread(src, file_size, 1, fh)) {
        fclose(fh);
        sl_error(vm, vm->lib.Error,
            "Could not load %Qs - %s",
            filename, strerror(errno));
    }
    fclose(fh);

    if(skip_hashbang) {
        real_start = get_hashbang_length(src, file_size);
    }

    return sl_do_string(vm,
        src + real_start,
        file_size - real_start,
        filename, 0);
}

static char*
sl_str_dupn(sl_vm_t* vm, char* str, size_t n)
{
    char* res = sl_alloc(vm->arena, n + 1);
    if(res) {
        strncpy(res, str, n);
        res[n] = 0;
    }

    return res;
}

static char*
env_to_http_header_name(sl_vm_t* vm, char* name, size_t len)
{
    char* result = sl_alloc(vm->arena, len + 1);
    char* str = result;

    *str++ = *name++;
    while (*name && (str - result) < len) {
        if (*name == '_') {
            *str++ = '-';
            name++;
            if (*name) {
                *str++ = *name++;
            }
        } else if (*name >= 'A' && *name <= 'Z') {
            *str++ = (*name++ - 'A' + 'a');
        } else {
            *str++ = *name++;
        }
    }
    *str = 0;

    return result;
}

#define SAPI_CGI_MAX_HEADER_LENGTH (1024)
#define SAPI_CGI_HEADER_SEPARATOR_LENGTH (2)
static const char* SAPI_CGI_HEADER_SEPARATOR  = "\r\n";
#define SAPI_CGI_HEADER_NAMEVALUE_SEPARATOR_LENGTH (2)
static const char* SAPI_CGI_HEADER_NAMEVALUE_SEPARATOR = ": ";

static void write_header(slash_context_t* ctx, char* name, char* value)
{
    int namelen = strlen(name);
    int valuelen = strlen(value);

    /* Emit the header withouth buffering. That way no buffer is requried
     * and there will be no problems if a long header is written.
     */
    ctx->api->write_out(ctx->api, name, namelen);

    ctx->api->write_out(ctx->api, SAPI_CGI_HEADER_NAMEVALUE_SEPARATOR,
        SAPI_CGI_HEADER_NAMEVALUE_SEPARATOR_LENGTH);

    ctx->api->write_out(ctx->api, value, valuelen);

    ctx->api->write_out(ctx->api, SAPI_CGI_HEADER_SEPARATOR,
        SAPI_CGI_HEADER_SEPARATOR_LENGTH);
}

#define SAPI_CGI_MAX_STATUS_LINE_LENGTH (64)
static void write_status(slash_context_t* ctx, int status) 
{
    char buff[SAPI_CGI_MAX_STATUS_LINE_LENGTH];
    int sl_len = http_status_format_line(status, buff, sizeof(buff));

    if(sl_len > 0 && sl_len <= sizeof(buff)) {
        write_header(ctx, "Status", buff);
    }
}

static void
flush_headers(slash_context_t* ctx)
{
#ifdef SL_TEST
    /* don't write the headers in testing mode */
    return;
#endif
    int status;
    sl_response_key_value_t* headers;
    size_t header_count, i;

    if(!ctx->headers_sent) {
        int content_type_sent = 0;
        ctx->headers_sent = 1;

        status = sl_response_get_status(ctx->vm);

        write_status(ctx, status);

        headers = sl_response_get_headers(ctx->vm, &header_count);
        for(i = 0; i < header_count; i++) {
            if(!content_type_sent &&
                strcasecmp("Content-Type", headers[i].name) == 0) {
                content_type_sent = 1;
            }
            write_header(ctx, headers[i].name, headers[i].value);
        }

        if(!content_type_sent) {
            write_header(ctx, "Content-Type", "text/html; charset=utf-8");
        }

        /* terminate headers */
        ctx->api->write_out(ctx->api, SAPI_CGI_HEADER_SEPARATOR,
            SAPI_CGI_HEADER_SEPARATOR_LENGTH);
    }
}

static void
fix_request_info(sl_vm_t* vm, cgi_options* options, slash_api_base_t* api, slash_request_info_t* info)
{
    info->real_uri = NULL;
    info->real_path_info = NULL;
    info->real_canonical_filename = NULL;
    info->real_canonical_dir = NULL;

    if(info->request_uri) {
        info->real_uri = info->request_uri->value;
    }

    if(info->path_info) {
        info->real_path_info = info->path_info->value;
    }

    if(info->script_filename) {
        info->real_canonical_filename = info->script_filename->value;
    }

    if(api->type == SLASH_REQUEST_FCGI) { 
        if(info->path_translated) {
            char* filename = info->path_translated->value;
            char* path_info = NULL;
            struct stat st;

            if(!sl_abs_file_exists(filename)) {
                size_t filename_len;
                char* ptr;
                filename_len = strlen(info->path_translated->value);

                filename = sl_str_dupn(vm,
                    info->path_translated->value, filename_len);

                while((ptr = strrchr(filename, '/'))
                    || (ptr = strrchr(filename, '\\'))) {
                    *ptr = 0;

                    if(sl_abs_file_exists(filename)) {
                        if(ptr > filename) {
                            path_info = 
                                &info->path_translated->value[ptr - filename];
                        }
                        break;
                    }
                }
            }

            if(stat(filename, &st) >= 0 && S_ISREG(st.st_mode)) {
                info->real_canonical_filename = filename;
                info->real_path_info = path_info;
            }
        }
    } else if(api->type == SLASH_REQUEST_CGI && options
        && options->arg_script_filename
        && !info->real_canonical_filename) {
        info->real_canonical_filename = options->arg_script_filename;
    }

    /* remove the query string from the URI if present */
    if(info->real_uri && info->real_uri[0]
        && info->query_string && info->query_string->value 
        && info->query_string->value[0]) {
        size_t uri_len, qs_len, real_uri_len;
        char* orig_uri = info->real_uri;
        char* qs = info->query_string->value;

        uri_len = strlen(info->real_uri);
        qs_len = strlen(qs);

        real_uri_len = (uri_len - qs_len - 1);

        if(uri_len > qs_len &&
            real_uri_len > 0 &&
            info->real_uri[real_uri_len] == '?') {
            info->real_uri = sl_str_dupn(vm, orig_uri, real_uri_len);
        }
    }

    /* calculate the directory name on the basis of the canonical filename */
    if(info->real_canonical_filename) {
        int filename_len;
        char* cwd, *last_slash;

        filename_len = strlen(info->real_canonical_filename);
        cwd = sl_str_dupn(vm, info->real_canonical_filename, filename_len);

        if((last_slash = strrchr(cwd, '/')) != NULL ||
            (last_slash = strrchr(cwd, '\\')) != NULL) {
            *last_slash = 0;
        }

        info->real_canonical_dir = cwd;
    }
}

static void
load_request_info(sl_vm_t* vm, cgi_options* options, slash_api_base_t* api,
    slash_request_info_t* result)
{
    char** env;

    result->environment = sl_request_key_value_list_new(vm, 30);
    result->http_headers = sl_request_key_value_list_new(vm, 10);
    result->script_name = NULL;
    result->path_translated = NULL;
    result->path_info = NULL;
    result->script_filename = NULL;
    result->content_length = NULL;
    result->content_type = NULL;
    result->request_uri = NULL;
    result->query_string = NULL;
    result->remote_addr = NULL;
    result->request_method = NULL;

    for(env = api->environ; env && *env != NULL; ++env) {
        sl_request_key_value_t* current;
        char* eq = strchr(*env, '=');

        if(eq == NULL) {
            continue;
        }

        current = sl_request_key_value_list_add(
            result->environment,
            sl_str_dupn(vm, *env, eq - *env),
            (char*)(eq + 1));

        if(strncmp(current->name, "HTTP_", 5) == 0) {
            sl_request_key_value_list_add(result->http_headers,
                env_to_http_header_name(vm,
                    current->name + 5, (eq - (*env + 5))
                ),
                current->value);
        } else if(strcmp(current->name, "CONTENT_LENGTH") == 0) {
            sl_request_key_value_list_add(result->http_headers,
                "Content-Length",
                current->value);
            result->content_length = current;
        } else if(strcmp(current->name, "CONTENT_TYPE") == 0) {
            sl_request_key_value_list_add(result->http_headers,
                "Content-Type",
                current->value);
            result->content_type = current;
        } else if(strcmp(current->name, "SCRIPT_NAME") == 0) {
            result->script_name = current;
        } else if(strcmp(current->name, "PATH_TRANSLATED") == 0) {
            result->path_translated = current;
        } else if(strcmp(current->name, "PATH_INFO") == 0) {
            result->path_info = current;
        } else if(strcmp(current->name, "SCRIPT_FILENAME") == 0) {
            result->script_filename = current;
        } else if(strcmp(current->name, "REQUEST_URI") == 0) {
            result->request_uri = current;
        } else if(strcmp(current->name, "QUERY_STRING") == 0) {
            result->query_string = current;
        } else if(strcmp(current->name, "REMOTE_ADDR") == 0) {
            result->remote_addr = current;
        } else if(strcmp(current->name, "REQUEST_METHOD") == 0) {
            result->request_method = current;
        }
    }

    fix_request_info(vm, options, api, result);
}

static bool
read_post_data(sl_vm_t* vm, sl_request_opts_t* opts, slash_request_info_t* info)
{
    bool result = true;
    slash_context_t* ctx = vm->data;
    size_t content_length = 0;

    opts->post_length = 0;

    if(info->content_length) {
        long long int cl;
        cl = strtoll(info->content_length->value, NULL, 10);

        /* check for underflow/overflow condition */
        if(!((cl == LLONG_MIN || cl == LLONG_MAX) && errno == ERANGE)) {
            if(cl > 0 && cl <= SIZE_MAX) {
                content_length = (size_t) cl;
            } else if(cl != 0) {
                result = false;
            }
        } else {
            result = false;
        }
    }

    if(content_length > 0) {
        size_t bytes_read = 0;
        opts->post_data = sl_alloc(vm->arena, content_length);

        bytes_read = ctx->api->read_in(ctx->api,
            opts->post_data, content_length);

        opts->post_length = bytes_read;
    }

    return result;
}

static bool
setup_request_object(sl_vm_t* vm, slash_request_info_t* info)
{
    bool result = true;
    sl_request_opts_t opts;

    opts.method       =
        info->request_method ? info->request_method->value : "GET";

    opts.uri          = info->real_uri ? info->real_uri : "";
    opts.path_info    = info->real_path_info ? info->real_path_info : "";
    opts.query_string = info->query_string ? info->query_string->value : "";
    opts.remote_addr  = info->remote_addr ? info->remote_addr->value : "";
    opts.content_type = info->content_type ? info->content_type->value : NULL;

    opts.header_count = info->http_headers->count;
    opts.headers      = info->http_headers->kvs;
    
    opts.env_count    = info->environment->count;
    opts.env          = info->environment->kvs;
    
    result = read_post_data(vm, &opts, info);

    sl_request_set_opts(vm, &opts);

    return result;
}

static void output(sl_vm_t* vm, char* buff, size_t len)
{
    slash_context_t* ctx = vm->data;
    flush_headers(ctx);
    ctx->api->write_out(ctx->api, buff, len);
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
load_cgi_options(sl_vm_t* vm, cgi_options* options) 
{
    int i = 0;
    for(; options->incpaths && i < options->incpaths_count; i++) {
        sl_require_path_prepend(vm, options->incpaths[i]);
    }

    SLVAL vargv = sl_make_array(vm, 0, NULL);
    for(i = 0; i < options->arg_script_options_count; i++) {
        SLVAL varg = sl_make_cstring(vm, options->arg_script_options[i]);
        sl_array_push(vm, vargv, 1, &varg);
    }
    sl_class_set_const(vm, vm->lib.Object, "ARGV", vargv);
}

static void
run_slash_script(slash_api_base_t* api, cgi_options* options, void* stack_top)
{
    sl_vm_t* vm;
    slash_context_t ctx;
    sl_vm_frame_t exit_frame, exception_frame;
    char* canonical_filename;
    SLVAL error;

    sl_static_init();
    vm = sl_init("cgi-fcgi");

    bzero(&ctx, sizeof(ctx));
    ctx.api = api;
    ctx.headers_sent = 0;
    ctx.vm = vm;
    vm->data = &ctx;

    sl_gc_set_stack_top(vm->arena, stack_top);

    load_cgi_options(vm, options);
    load_request_info(vm, options, api, &ctx.info);

#ifdef SL_TEST
    register_cgi_test_utils(vm);
#endif

    canonical_filename = ctx.info.real_canonical_filename;
#ifndef SL_TEST
    vm->cwd = ctx.info.real_canonical_dir;
#endif

    if(canonical_filename) {
        SL_TRY(exit_frame, SL_UNWIND_ALL, {
            SL_TRY(exception_frame, SL_UNWIND_EXCEPTION, {
                bool request_valid = setup_request_object(vm, &ctx.info);

                setup_response_object(vm);

                if(!request_valid) {
                    sl_error(vm, vm->lib.Error, "Invalid Request");
                }

                sl_do_file_hashbang(vm, canonical_filename,
                    api->type == SLASH_REQUEST_CGI);
            }, error, {
                sl_response_clear(vm);
                sl_render_error_page(vm, error);
            });
        }, error, {});

        flush_headers(&ctx);
        sl_response_flush(vm);
    }
    sl_free_gc_arena(vm->arena);
}

static void
print_help(char* program_name)
{
    fprintf(stderr, "Usage: %s [options]\n", program_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");

    fprintf(stderr, "   -I <path>       Adds <path> to the list of paths"
                    " searched when requiring files\n");
    fprintf(stderr, "   -b <path,:port> FCGI: Listen on the given path\n");
    fprintf(stderr, "   -B <backlog>    FCGI: Size of listen queue\n");
    fprintf(stderr, "   -c <number>     FCGI: Number of children to fork\n");
    fprintf(stderr, "   -h              Prints this help\n");
}


static void 
add_incpath_option(cgi_options* options, char* path)
{
    if(options->incpaths_count == options->incpaths_capacity) {
        if(options->incpaths_capacity == 0) {
            options->incpaths_capacity = 5;
        } else {
            options->incpaths_capacity *= 2;
        }
        options->incpaths = realloc(options->incpaths,
            options->incpaths_capacity * sizeof(char*));
    }

    if(options->incpaths) {
        options->incpaths[options->incpaths_count++] = path;
    }
}

static void
add_script_option(cgi_options* options, char* option)
{
    if(options->arg_script_options_count ==
        options->arg_script_options_capacity) {
        if(options->arg_script_options_capacity == 0) {
            options->arg_script_options_capacity = 5;
        } else {
            options->arg_script_options_capacity *= 2;
        }
        options->arg_script_options = realloc(options->arg_script_options,
            options->arg_script_options_capacity * sizeof(char*));
    }

    if(options->arg_script_options) {
        options->arg_script_options[options->arg_script_options_count++] =
            option;
    }
}

static void 
init_options(cgi_options* options, size_t incpaths_capacity) 
{
    int env_int;
    char* env_raw;
    bzero(options, sizeof(cgi_options));

    options->fcgi_backlog = 5;

    if(incpaths_capacity > 0) {
        options->incpaths = malloc(sizeof(char*) * incpaths_capacity);
        if(options->incpaths) {
            options->incpaths_capacity = incpaths_capacity;
        }
    }

    if((env_raw = getenv("SL_FCGI_CHILDREN")) != NULL) {
        env_int = atoi(env_raw);
        if(env_int > 0) {
            options->fcgi_num_childs = env_int;
        }
    }

    if((env_raw = getenv("SL_FCGI_MAX_REQUESTS")) != NULL) {
        env_int = atoi(env_raw);
        if(env_int > 0) {
            options->fcgi_num_requests = env_int;
        }
    }
}

static void
cleanup_options(cgi_options* options)
{
    if(!options) {
        return;
    }

    free(options->incpaths);
    options->incpaths = NULL;
}

static void 
process_arguments_exit(cgi_options* options, int retval) {
    cleanup_options(options);
    exit(retval);
}

static void
process_arguments(cgi_options* options, int argc, char** argv)
{
    int i = 1;
    for(; i < argc; i++) {
        if(strcmp(argv[i], "-I") == 0) {
            if(i + 1 == argc) {
                fprintf(stderr, "Expected <path> after -I\n");
                process_arguments_exit(options, 1);
            }
            add_incpath_option(options, argv[++i]);
        } else if(strcmp(argv[i], "-b") == 0) {
            if(i + 1 == argc) {
                fprintf(stderr, "Expected socket or port number after -b\n");
                process_arguments_exit(options, 1);
            }
            options->fcgi_bind = argv[++i];
        } else if(strcmp(argv[i], "-B") == 0) {
            int backlog;
            if(i + 1 == argc) {
                fprintf(stderr, "Expected numeric backlog count after -B\n");
                process_arguments_exit(options, 1);
            }
            backlog = atoi(argv[++i]);

            if(backlog <= 0) {
                fprintf(stderr, "Expected numeric backlog count after -B\n");
                process_arguments_exit(options, 1);
            }

            options->fcgi_backlog = backlog;
        } else if(strcmp(argv[i], "-c") == 0) {
            int childcount;
            if(i + 1 == argc) {
                fprintf(stderr, "Expected numeric child count after -c\n");
                process_arguments_exit(options, 1);
            }
            childcount = atoi(argv[++i]);

            if(childcount <= 0) {
                fprintf(stderr, "Expected numeric child count after -c\n");
                process_arguments_exit(options, 1);
            }

            options->fcgi_num_childs = childcount;
        } else if(strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            process_arguments_exit(options, 0);
        } else if(strcmp(argv[i], "--") == 0) {
            break;
        } else {
            break;
        }
    }

    if(i < argc) {
        options->arg_script_filename = argv[i++];

        for(; i < argc; ++i) {
            add_script_option(options, argv[i]);
        }
    }
}


static void fcgi_child_main(int socket, cgi_options* options) {
    int requests = 0;
    FCGX_Request request;
    slash_api_base_t* api;

    FCGX_InitRequest(&request, socket, 0);
    while((options->fcgi_num_requests == 0 ||
            (requests++ < options->fcgi_num_requests)) &&
            FCGX_Accept_r(&request) >= 0) {

        api = slash_api_fcgi_new(request.out, request.err, request.in, request.envp);
        run_slash_script(api, options, &api);
        slash_api_free(api);
        FCGX_Finish_r(&request);
    }
}

struct sigaction act, old_term, old_quit, old_int;
static pid_t pgroup;
static int parent = 1;

static void fcgi_cleanup(int signal)
{
#ifdef DEBUG_FASTCGI
    fprintf(stderr, "slash-cgi: FastCGI shutdown, pid %d\n", getpid());
#endif

    sigaction(SIGTERM, &old_term, 0);

    /* Kill all the processes in our process group */
    kill(-pgroup, SIGTERM);

    if (parent) {
        /*exit_signal = 1;*/
    } else {
        exit(0);
    }
}

static void fcgi_parent_main(int socket, cgi_options* options) {
    int children = 0, status = 0;
    pid_t pid;

    setsid();
    pgroup = getpgrp();
#ifdef DEBUG_FASTCGI
    fprintf(stderr, "slash-cgi: Process group %d\n", pgroup);
#endif

    act.sa_flags = 0;
    act.sa_handler = fcgi_cleanup;
    if (sigaction(SIGTERM, &act, &old_term) ||
        sigaction(SIGINT,  &act, &old_int)  ||
        sigaction(SIGQUIT, &act, &old_quit)
    ) {
        fprintf(stderr, "slash-cgi: can't set signals\n");
        return;
    }

    while(parent) {
        for(; children < options->fcgi_num_childs; ++children) {
            pid = fork();

            if(pid < 0) {
                fprintf(stderr, "slash-cgi: fork failed!\n");
                return;
            } else if(pid == 0) {
                parent = 0;
                sigaction(SIGTERM, &old_term, 0);
                sigaction(SIGQUIT, &old_quit, 0);
                sigaction(SIGINT,  &old_int,  0);
                break;
            }
        }

        if(parent) {
#ifdef DEBUG_FASTCGI
            fprintf(stderr, "slash-cgi: Wait for kids, pid %d\n", getpid());
#endif
            while(1) {
                if(wait(&status) >= 0) {
                    --children;
                    break;
                }
            }
        } else {
#ifdef DEBUG_FASTCGI
            fprintf(stderr, "slash-cgi: Start of child, pid %d\n", getpid());
#endif

            fcgi_child_main(socket, options);

#ifdef DEBUG_FASTCGI
            fprintf(stderr, "slash-cgi: End of child, pid %d\n", getpid());
#endif
        }
    }
}

int
main(int argc, char** argv)
{
    int socket;
    cgi_options options;

    init_options(&options, argc > 2 ? 5 : 0);
    process_arguments(&options, argc, argv);

    if(options.fcgi_bind == NULL && FCGX_IsCGI()) {
        slash_api_base_t* api;
        api = slash_api_cgi_new(stdout, stderr, stdin, environ);
        run_slash_script(api, &options, &api);
        slash_api_free(api);
    } else {
        FCGX_Init();
        if(options.fcgi_bind && 
            options.fcgi_backlog > 0) {
            socket = FCGX_OpenSocket(options.fcgi_bind, options.fcgi_backlog);

            if(socket < 0) {
                fprintf(stderr, "slash-cgi: FCGX_OpenSocket failed!\n");
                cleanup_options(&options);
                return 1;
            }
        } else {
            socket = 0;
        }

        if(options.fcgi_num_childs > 0) {
            fcgi_parent_main(socket, &options);
        } else {
            fcgi_child_main(socket, &options);
        }
    }

    cleanup_options(&options);

    return 0;
}
