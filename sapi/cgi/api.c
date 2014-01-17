#include <fcgiapp.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "api.h"

void slash_api_free(slash_api_base_t* api)
{
    free(api);
}

typedef struct slash_cgi_api {
    slash_api_base_t base;
    FILE* out;
    FILE* err;
    FILE* in;
} slash_cgi_api_t;

static size_t
slash_api_cgi_writeOut(slash_api_base_t* self, const char* str, size_t n)
{
    slash_cgi_api_t* cgiSelf = (slash_cgi_api_t*)self;

    return fwrite(str, n, 1, cgiSelf->out) * n;
}

static size_t
slash_api_cgi_writeErr(slash_api_base_t* self, const char* str, size_t n)
{
    slash_cgi_api_t* cgiSelf = (slash_cgi_api_t*)self;

    return fwrite(str, n, 1, cgiSelf->err) * n;
}

static size_t
slash_api_cgi_readIn(slash_api_base_t* self, char* str, size_t n)
{
    slash_cgi_api_t* cgiSelf = (slash_cgi_api_t*)self;

    return fread(str, n, 1, cgiSelf->in) * n;
}

slash_api_base_t* slash_api_cgi_new(FILE* out, FILE* err, FILE* in, char** envp)
{
    slash_cgi_api_t* result = malloc(sizeof(slash_cgi_api_t));

    if(result) {
        result->base.type = SLASH_REQUEST_CGI;
        result->base.environ = envp;
        result->base.write_out = slash_api_cgi_writeOut;
        result->base.write_err = slash_api_cgi_writeErr;
        result->base.read_in = slash_api_cgi_readIn;
        result->out = out;
        result->err = err;
        result->in = in;
    }

    return (slash_api_base_t*)result;
}

typedef struct slash_fcgi_api {
    slash_api_base_t base;
    FCGX_Stream* out;
    FCGX_Stream* err;
    FCGX_Stream* in;
} slash_fcgi_api_t;

static size_t
slash_api_fcgi_write_stream(FCGX_Stream* stream, const char* str, size_t n)
{
    size_t result = 0;

    while(n > 0) {
        int towrite = n > INT_MAX ? INT_MAX : n;
        int written = FCGX_PutStr(str + result, towrite, stream);

        if(written <= 0) {
            break;
        } else {
            result += written;
            n -= written;
        }
    }

    return result;
}
static size_t
slash_api_fcgi_writeOut(slash_api_base_t* self, const char* str, size_t n)
{
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;
    return slash_api_fcgi_write_stream(fcgiSelf->out, str, n);
}

static size_t
slash_api_fcgi_writeErr(slash_api_base_t* self, const char* str, size_t n)
{
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;
    return slash_api_fcgi_write_stream(fcgiSelf->err, str, n);
}

static size_t
slash_api_fcgi_readIn(slash_api_base_t* self, char* str, size_t n)
{
    size_t result = 0;
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;

    while(n > 0) {
        int toread = n > INT_MAX ? INT_MAX : n;
        int read = FCGX_GetStr(str + result, toread, fcgiSelf->in);

        if(read > 0) {
            result += read;
            n -= read;
        }

        if(read < toread) {
            /* EOF reached! */
            break;
        }
    }

    return result;
}

slash_api_base_t* slash_api_fcgi_new(FCGX_Stream* out, FCGX_Stream* err,
    FCGX_Stream* in, char** envp)
{
    slash_fcgi_api_t* result = malloc(sizeof(slash_fcgi_api_t));

    if(result) {
        result->base.type = SLASH_REQUEST_FCGI;
        result->base.environ = envp;
        result->base.write_out = slash_api_fcgi_writeOut;
        result->base.write_err = slash_api_fcgi_writeErr;
        result->base.read_in = slash_api_fcgi_readIn;
        result->out = out;
        result->err = err;
        result->in = in;
    }

    return (slash_api_base_t*)result;
}

