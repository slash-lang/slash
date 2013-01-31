#ifndef SL_LIB_BUFFER_H
#define SL_LIB_BUFFER_H

SLVAL
sl_make_buffer(sl_vm_t* vm, char* data, size_t len);

SLVAL
sl_make_buffer2(sl_vm_t* vm, char* data, size_t len, size_t capacity);

#endif
