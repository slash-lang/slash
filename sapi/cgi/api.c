#include <fcgiapp.h>
#include <stdlib.h>
#include <stdio.h>
#include "api.h"

void slash_api_free(slash_api_base_t * api)
{
    free(api);
}

typedef struct slash_cgi_api {
    slash_api_base_t base;
    FILE * out;
    FILE * err;
    FILE * in;
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

slash_api_base_t * slash_api_cgi_new(FILE* out, FILE* err, FILE* in, char** envp)
{
    slash_cgi_api_t * result = malloc(sizeof(slash_cgi_api_t));

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
    FCGX_Stream * out;
    FCGX_Stream * err;
    FCGX_Stream * in;
} slash_fcgi_api_t;

static size_t
slash_api_fcgi_writeOut(slash_api_base_t* self, const char* str, size_t n)
{
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;

    /* TODO: if n > MAX_INT split the call */
    return FCGX_PutStr(str, (int)n, fcgiSelf->out);
}

static size_t
slash_api_fcgi_writeErr(slash_api_base_t* self, const char* str, size_t n)
{
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;

    /* TODO: if n > MAX_INT split the call */
    return FCGX_PutStr(str, (int)n, fcgiSelf->err);
}

static size_t
slash_api_fcgi_readIn(slash_api_base_t* self, char* str, size_t n)
{
    slash_fcgi_api_t* fcgiSelf = (slash_fcgi_api_t*)self;

    /* TODO: FCGX_GetStr expects len as `int`. In case len > MAX_INT split up
     * the call in two calls or whatever.
     */
    return FCGX_GetStr(str, n, fcgiSelf->in);
}

slash_api_base_t * slash_api_fcgi_new(FCGX_Stream * out, FCGX_Stream* err,
    FCGX_Stream* in, char ** envp)
{
    slash_fcgi_api_t * result = malloc(sizeof(slash_fcgi_api_t));

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

