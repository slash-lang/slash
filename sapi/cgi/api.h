#ifndef SL_SAPI_CGI_API
#define SL_SAPI_CGI_API

typedef enum {
    SLASH_REQUEST_CGI
    ,SLASH_REQUEST_FCGI
} slash_api_type_t;

typedef struct slash_api_base {
    slash_api_type_t type;
    char ** environ;

    size_t (*write_out)(struct slash_api_base* self, const char * str, size_t n);
    size_t (*write_err)(struct slash_api_base* self, const char * str, size_t n);
    size_t (*read_in)(struct slash_api_base* self, char * str, size_t n);

} slash_api_base_t;

void
slash_api_free(slash_api_base_t * api);


slash_api_base_t*
slash_api_cgi_new(FILE* out,
                  FILE* err,
                  FILE* in,
                  char** envp);

slash_api_base_t*
slash_api_fcgi_new(FCGX_Stream * out,
                   FCGX_Stream* err,
                   FCGX_Stream* in,
                   char ** envp);

#endif /*  SL_SAPI_CGI_API */
