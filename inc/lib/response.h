#ifndef SL_LIB_RESPONSE_H
#define SL_LIB_RESPONSE_H

#include "vm.h"
#include "value.h"

typedef struct {
    char* name;
    char* value;
}
sl_response_key_value_t;

typedef struct {
    int status;
    size_t header_count;
    sl_response_key_value_t* headers;
    int buffered;
    SLVAL output_buffer;
    void(*write)(sl_vm_t*,char*,size_t);
}
sl_response_opts_t;

void
sl_response_set_opts(sl_vm_t* vm, sl_response_opts_t* opts);

#endif
