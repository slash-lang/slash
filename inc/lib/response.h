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
    int descriptive_error_page;
    int buffered;
    void(*write)(sl_vm_t*,char*,size_t);
}
sl_response_opts_t;

void
sl_response_set_opts(sl_vm_t* vm, sl_response_opts_t* opts);

SLVAL
sl_response_write(sl_vm_t* vm, SLVAL str);

SLVAL
sl_response_flush(sl_vm_t* vm);

void
sl_response_clear(sl_vm_t* vm);

sl_response_key_value_t*
sl_response_get_headers(sl_vm_t* vm, size_t* count);

int
sl_response_get_status(sl_vm_t* vm);

void
sl_render_error_page(sl_vm_t* vm, SLVAL err);

#endif
