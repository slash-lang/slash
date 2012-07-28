#ifndef SL_PLATFORM_H
#define SL_PLATFORM_H

char* sl_realpath(char* path);
int sl_file_exists(char* path);
int sl_seed();
void* sl_stack_limit();

#endif
