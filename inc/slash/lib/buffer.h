#ifndef SL_LIB_BUFFER_H
#define SL_LIB_BUFFER_H

#include <stdint.h>
#include <slash.h>

SLVAL
sl_make_buffer(sl_vm_t* vm, char* data, size_t len);

SLVAL
sl_make_buffer2(sl_vm_t* vm, char* data, size_t len, size_t capacity);

typedef struct {
    void* data;
    size_t len;
}
sl_frozen_buffer_t;

sl_frozen_buffer_t*
sl_buffer_get_frozen_copy(sl_vm_t* vm, SLVAL buffer);

#endif
