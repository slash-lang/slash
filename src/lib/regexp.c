#include "lib/regexp.h"
#include "class.h"
#include "string.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <pcre.h>

typedef struct {
    sl_object_t base;
    pcre* re;
    pcre_extra* study;
    int options;
}
sl_regexp_t;

typedef struct {
    sl_object_t base;
    sl_regexp_t* re;
    SLVAL match_string;
    int* captures;
    int capture_count;
}
sl_regexp_match_t;

static sl_regexp_t*
get_regexp(sl_vm_t* vm, SLVAL re)
{
    sl_expect(vm, re, vm->lib.Regexp);
    return (sl_regexp_t*)sl_get_ptr(re);
}

static sl_regexp_t*
get_regexp_check(sl_vm_t* vm, SLVAL rev)
{
    sl_regexp_t* re = get_regexp(vm, rev);
    if(!re->re) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid operation on uninitialized regexp");
    }
    return re;
}

static sl_regexp_match_t*
get_regexp_match(sl_vm_t* vm, SLVAL matchv)
{
    sl_regexp_match_t* match;
    sl_expect(vm, matchv, vm->lib.Regexp_Match);
    match = (sl_regexp_match_t*)sl_get_ptr(matchv);
    if(!match->re) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid Regexp::Match");
    }
    return match;
}

static void
free_regexp(sl_regexp_t* re)
{
    /*
    if(re->study) {
        pcre_free_study(re->study);
    }
    */
    if(re->re) {
        pcre_free(re->re);
    }
}

static sl_object_t*
allocate_regexp()
{
    sl_object_t* re = GC_MALLOC(sizeof(sl_regexp_t));
    GC_register_finalizer(re, (void(*)(void*,void*))free_regexp, NULL, NULL, NULL);
    return re;
}

static sl_object_t*
allocate_regexp_match()
{
    return GC_MALLOC(sizeof(sl_regexp_match_t));
}

static void
sl_setup_regexp(sl_vm_t* vm, sl_regexp_t* re_ptr, uint8_t* re_buff, size_t re_len, uint8_t* opts_buff, size_t opts_len)
{
    char buff[256];
    const char* error;
    char* rez;
    int error_offset;
    int opts = PCRE_DOLLAR_ENDONLY | PCRE_UTF8;
    pcre* re;
    size_t i;
    for(i = 0; i < opts_len; i++) {
        switch(opts_buff[i]) {
            case 'i':
                opts |= PCRE_CASELESS;
                break;
            case 'x':
                opts |= PCRE_EXTENDED;
                break;
            default:
                sprintf(buff, "Unknown regular expression option '%c'", opts_buff[i]);
                sl_throw_message2(vm, vm->lib.SyntaxError, buff);
        }
    }
    if(memchr(re_buff, 0, re_len)) {
        sl_throw_message2(vm, vm->lib.SyntaxError, "Regular expression contains null byte");
    }
    rez = GC_MALLOC(re_len + 1);
    memcpy(rez, re_buff, re_len);
    rez[re_len] = 0;
    re = pcre_compile(rez, opts, &error, &error_offset, NULL);
    if(!re) {
        sl_throw_message2(vm, vm->lib.SyntaxError, (char*)error);
    }
    if(re_ptr->re) {
        /*
        if(re_ptr->study) {
            pcre_free_study(re_ptr->study);
        }
        */
        pcre_free(re_ptr->re);
    }
    re_ptr->options = opts;
    re_ptr->re = re;
    re_ptr->study = NULL;
}

SLVAL
sl_make_regexp(sl_vm_t* vm, uint8_t* re_buff, size_t re_len, uint8_t* opts_buff, size_t opts_len)
{
    SLVAL rev = sl_allocate(vm, vm->lib.Regexp);
    sl_regexp_t* re_ptr = get_regexp(vm, rev);
    sl_setup_regexp(vm, re_ptr, re_buff, re_len, opts_buff, opts_len);
    return rev;
}

static SLVAL
sl_regexp_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_regexp_t* re_ptr = get_regexp(vm, self);
    sl_string_t* re = (sl_string_t*)sl_get_ptr(sl_expect(vm, argv[0], vm->lib.String));
    sl_string_t* opts;
    if(argc > 1) {
        opts = (sl_string_t*)sl_get_ptr(sl_expect(vm, argv[0], vm->lib.String));
    } else {
        opts = sl_cstring(vm, "");
    }
    sl_setup_regexp(vm, re_ptr, re->buff, re->buff_len, opts->buff, opts->buff_len);
    return self;
}

/*
static SLVAL
sl_regexp_compile(sl_vm_t* vm, SLVAL self)
{
    sl_regexp_t* re = get_regexp_check(vm, self);
    if(re->study) {
        return vm->lib._false;
    }
    re->study = pcre_study(re->re, PCRE_STUDY_JIT_COMPILE, NULL);
    return vm->lib._true;
}
*/

static SLVAL
sl_regexp_match(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_regexp_t* re = get_regexp_check(vm, self);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(sl_expect(vm, argv[0], vm->lib.String));
    int offset = 0, rc, ncaps;
    int* caps;
    char err_buff[256];
    sl_regexp_match_t* match;
    if(argc > 1) {
        offset = sl_get_int(sl_expect(vm, argv[1], vm->lib.Int));
    }
    offset = sl_string_byte_offset_for_index(vm, argv[0], offset);
    pcre_fullinfo(re->re, re->study, PCRE_INFO_CAPTURECOUNT, &ncaps);
    ncaps += 1;
    ncaps *= 3;
    caps = GC_MALLOC(sizeof(int) * ncaps);
    rc = pcre_exec(re->re, re->study, (char*)str->buff, str->buff_len, offset, PCRE_NEWLINE_LF, caps, ncaps);
    if(rc < 0) {
        if(rc == PCRE_ERROR_NOMATCH) {
            return vm->lib.nil;
        }
        if(rc == PCRE_ERROR_BADUTF8) {
            sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 in regular expression or match text");
        }
        sprintf(err_buff, "PCRE error (%d)", rc);
        sl_throw_message2(vm, vm->lib.Error, err_buff);
    }
    match = (sl_regexp_match_t*)sl_get_ptr(sl_allocate(vm, vm->lib.Regexp_Match));
    match->re = re;
    match->match_string = argv[0];
    match->capture_count = ncaps / 3;
    match->captures = caps;
    return sl_make_ptr((sl_object_t*)match);
}

static SLVAL
sl_regexp_match_regexp(sl_vm_t* vm, SLVAL self)
{
    return sl_make_ptr((sl_object_t*)get_regexp_match(vm, self)->re);
}

static SLVAL
sl_regexp_match_index(sl_vm_t* vm, SLVAL self, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = sl_get_int(sl_expect(vm, i, vm->lib.Int));
    sl_string_t* str = (sl_string_t*)sl_get_ptr(match->match_string);
    if(index < 0 || index >= match->capture_count) {
        return vm->lib.nil;
    }
    index *= 2;
    return sl_make_string(vm, str->buff + match->captures[index], match->captures[index + 1] - match->captures[index]);
}

static SLVAL
sl_regexp_match_length(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, get_regexp_match(vm, self)->capture_count);
}

void
sl_init_regexp(sl_vm_t* vm)
{
    vm->lib.Regexp = sl_define_class(vm, "Regexp", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Regexp, allocate_regexp);
    vm->lib.Regexp_Match = sl_define_class3(vm, sl_make_cstring(vm, "Match"), vm->lib.Object, vm->lib.Regexp);
    sl_class_set_allocator(vm, vm->lib.Regexp_Match, allocate_regexp_match);
    
    sl_define_method(vm, vm->lib.Regexp, "init", -2, sl_regexp_init);
    /*
    sl_define_method(vm, vm->lib.Regexp, "compile", 0, sl_regexp_compile);
    */
    sl_define_method(vm, vm->lib.Regexp, "match", -2, sl_regexp_match);
    
    sl_define_method(vm, vm->lib.Regexp_Match, "regexp", 0, sl_regexp_match_regexp);
    sl_define_method(vm, vm->lib.Regexp_Match, "[]", 1, sl_regexp_match_index);
    sl_define_method(vm, vm->lib.Regexp_Match, "length", 0, sl_regexp_match_length);
}
