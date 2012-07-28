#ifndef SL_LIB_REQUEST_H
#define SL_LIB_REQUEST_H

#include "value.h"
#include "vm.h"

typedef struct {
    char* name;
    char* value;
}
sl_request_key_value_t;

typedef struct {
    char* method;
    char* uri;
    char* remote_addr;
    size_t header_count;
    sl_request_key_value_t* headers;
    size_t get_count;
    sl_request_key_value_t* get_params;
    size_t post_count;
    sl_request_key_value_t* post_params;
}
sl_request_opts_t;

void
sl_request_set_opts(sl_vm_t* vm, sl_request_opts_t* opts);

#endif
