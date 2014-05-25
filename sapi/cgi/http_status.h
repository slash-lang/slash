#ifndef SL_SAPI_CGI_HTTP_STATUS
#define SL_SAPI_CGI_HTTP_STATUS

const char*
http_status_get_status_text(int code);

int
http_status_format_line(int code, char* buff, size_t len);

#endif /*  SL_SAPI_CGI_HTTP_STATUS */
