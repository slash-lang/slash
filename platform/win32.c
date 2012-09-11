#include <slash/platform.h>

#include <Windows.h>
#include <Wincrypt.h>
#include <stdlib.h>

char*
sl_realpath(sl_vm_t* vm, char* path)
{
    char *cpath, *gcbuff;
    if(!path[0] || path[1] != ':') {
        gcbuff = sl_alloc_buffer(vm->arena, strlen(vm->cwd) + strlen(path) + 10);
        strcpy(gcbuff, vm->cwd);
        strcat(gcbuff, "/");
        strcat(gcbuff, path);
        path = gcbuff;
    }

    #ifdef _MAX_PATH 
        cpath = sl_alloc_buffer(vm->arena, _MAX_PATH + 1);
        cpath = _fullpath(cpath, path, _MAX_PATH);
        return cpath;
    #else
        cpath = _fullpath(NULL, path, 0);
        gcbuff = sl_alloc_buffer(vm->arena, strlen(cpath) + 1);
        strcpy(gcbuff, cpath);
        free(cpath);
        return gcbuff;
    #endif
}

int
sl_file_exists(sl_vm_t* vm, char* path)
{
    DWORD dwAttrib = GetFileAttributes(sl_realpath(vm, path));
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int
sl_abs_file_exists(char* path)
{
    DWORD dwAttrib;
    if(!path[0] || path[1] != ':') return FALSE;
    dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static void
get_timeval( long* tv_sec, long* tv_usec )
{
    FILETIME ft;
    ULONGLONG tr;

    GetSystemTimeAsFileTime(&ft);

    tr = ft.dwHighDateTime;
    tr = tr << 32;
    tr |= ft.dwLowDateTime;
    tr = (tr - 11644473600000000ULL) / 10;
    *tv_sec = (long)(tr / 1000000UL);
    *tv_usec = (long)(tr % 1000000UL);
}

int
sl_seed()
{
    HCRYPTPROV hCryptProv;
    int seed;

    if(CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hCryptProv, sizeof(int), (void*)&seed);
        CryptReleaseContext(hCryptProv, 0);
    } else {
        long tv_sec, tv_usec;
        get_timeval(&tv_sec, &tv_usec);
        seed = (int)(tv_usec ^ tv_sec);
    }

    return seed;
}

void*
sl_stack_limit()
{
    /* TODO: VirtualQuery dwTopOfStack for length */
    return NULL;
}