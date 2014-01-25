#include <slash/lib/regexp.h>
#include <slash/lib/array.h>
#include <slash/class.h>
#include <slash/string.h>
#include <slash/object.h>
#include <string.h>
#include <stdio.h>
#include <pcre.h>

static const int DEFAULT_OPTIONS = PCRE_DOLLAR_ENDONLY | PCRE_UTF8;

typedef struct {
    sl_object_t base;
    SLVAL source;
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
free_regexp(void* ptr)
{
    sl_regexp_t* re = ptr;
    /*
    if(re->study) {
        pcre_free_study(re->study);
    }
    */
    if(re->re) {
        pcre_free(re->re);
    }
}

static sl_gc_shape_t
regexp_shape = {
    .mark     = sl_gc_conservative_mark,
    .finalize = free_regexp,
};

static sl_object_t*
allocate_regexp(sl_vm_t* vm)
{
    return sl_alloc2(vm->arena, &regexp_shape, sizeof(sl_regexp_t));
}

static sl_object_t*
allocate_regexp_match(sl_vm_t* vm)
{
    return sl_alloc(vm->arena, sizeof(sl_regexp_match_t));
}

static void
sl_setup_regexp(sl_vm_t* vm, sl_regexp_t* re_ptr, uint8_t* re_buff, size_t re_len, uint8_t* opts_buff, size_t opts_len)
{
    char buff[256];
    const char* error;
    char* rez;
    int error_offset;
    int opts = DEFAULT_OPTIONS;
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
                sl_throw_message2(vm, vm->lib.ArgumentError, buff);
        }
    }
    if(memchr(re_buff, 0, re_len)) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Regular expression contains null byte");
    }
    rez = sl_alloc_buffer(vm->arena, re_len + 1);
    memcpy(rez, re_buff, re_len);
    rez[re_len] = 0;
    re = pcre_compile(rez, opts, &error, &error_offset, NULL);
    if(!re) {
        sl_throw_message2(vm, vm->lib.SyntaxError, (char*)error);
    }
    re_ptr->source = sl_make_string(vm, re_buff, re_len);
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
    sl_string_t* re = sl_get_string(vm, argv[0]);
    sl_string_t* opts;
    if(argc > 1) {
        opts = sl_get_string(vm, argv[1]);
    } else {
        opts = sl_cstring(vm, "");
    }
    if(re_ptr->re) {
        sl_throw_message2(vm, vm->lib.TypeError, "Cannot reinitialize already initialized Regexp");
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

static void
check_pcre_error(sl_vm_t* vm, int rc)
{
    if(rc == PCRE_ERROR_BADUTF8) {
        sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 in regular expression or match text");
    }
    if(rc < 0) {
        sl_error(vm, vm->lib.Error, "PCRE error (%d)", rc);
    }
}

SLVAL
sl_regexp_match(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_regexp_t* re = get_regexp_check(vm, self);
    sl_string_t* str = sl_get_string(vm, argv[0]);
    int offset = 0, rc, ncaps;
    int* caps;
    sl_regexp_match_t* match;
    if(argc > 1) {
        offset = sl_get_int(sl_expect(vm, argv[1], vm->lib.Int));
    }
    offset = sl_string_byte_offset_for_index(vm, argv[0], offset);
    if(offset < 0) {
        return vm->lib.nil;
    }
    pcre_fullinfo(re->re, re->study, PCRE_INFO_CAPTURECOUNT, &ncaps);
    ncaps += 1;
    ncaps *= 3;
    caps = sl_alloc(vm->arena, sizeof(int) * ncaps);
    rc = pcre_exec(re->re, re->study, (char*)str->buff, str->buff_len, offset, PCRE_NEWLINE_LF, caps, ncaps);
    if(rc == PCRE_ERROR_NOMATCH) {
        return vm->lib.nil;
    }
    check_pcre_error(vm, rc);
    match = (sl_regexp_match_t*)sl_get_ptr(sl_allocate(vm, vm->lib.Regexp_Match));
    match->re = re;
    match->match_string = argv[0];
    match->capture_count = ncaps / 3;
    match->captures = caps;
    return sl_make_ptr((sl_object_t*)match);
}

SLVAL
sl_regexp_is_match(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_regexp_t* re = get_regexp_check(vm, self);
    sl_string_t* str = sl_get_string(vm, other);
    int rc = pcre_exec(re->re, re->study, (char*)str->buff, str->buff_len, 0, PCRE_NEWLINE_LF, NULL, 0);
    if(rc == PCRE_ERROR_NOMATCH) {
        return vm->lib._false;
    }
    check_pcre_error(vm, rc);
    return vm->lib._true;
}

static SLVAL
sl_regexp_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(!sl_is_a(vm, other, vm->lib.Regexp)) {
        return vm->lib._false;
    }
    sl_regexp_t* re = get_regexp(vm, self);
    sl_regexp_t* oth = get_regexp(vm, other);
    if(!oth->re || !re->re) {
        return vm->lib._false;
    }
    if(!sl_is_truthy(sl_string_eq(vm, re->source, oth->source))) {
        return vm->lib._false;
    }
    if(re->options != oth->options) {
        return vm->lib._false;
    }
    return vm->lib._true;
}

static SLVAL
sl_regexp_source(sl_vm_t* vm, SLVAL self)
{
    return get_regexp_check(vm, self)->source;
}

static SLVAL
sl_regexp_options(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, get_regexp_check(vm, self)->options & ~DEFAULT_OPTIONS);
}

static SLVAL
sl_regexp_match_regexp(sl_vm_t* vm, SLVAL self)
{
    return sl_make_ptr((sl_object_t*)get_regexp_match(vm, self)->re);
}

static int
cap_index(sl_vm_t* vm, SLVAL regexp_match, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, regexp_match);
    int index;
    if(sl_is_a(vm, i, vm->lib.String)) {
        char* named_cap = sl_to_cstr(vm, i);
        index = pcre_get_stringnumber(match->re->re, named_cap);
        if(index < 0) {
            return -1;
        }
    } else {
        index = sl_get_int(sl_expect(vm, i, vm->lib.Int));
    }
    if(index < 0 || index >= match->capture_count) {
        return -1;
    }
    return index * 2;
}

SLVAL
sl_regexp_match_byte_offset(sl_vm_t* vm, SLVAL self, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, i);
    if(index < 0) {
        return vm->lib.nil;
    }
    return sl_make_int(vm, match->captures[index]);
}

SLVAL
sl_regexp_match_index(sl_vm_t* vm, SLVAL self, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, i);
    if(index < 0) {
        return vm->lib.nil;
    }
    sl_string_t* str = (sl_string_t*)sl_get_ptr(match->match_string);
    return sl_make_string(vm, str->buff + match->captures[index], match->captures[index + 1] - match->captures[index]);
}

SLVAL
sl_regexp_match_offset(sl_vm_t* vm, SLVAL self, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, i);
    if(index < 0) {
        return vm->lib.nil;
    }
    int offset = match->captures[index];
    return sl_make_int(vm, sl_string_index_for_byte_offset(vm, match->match_string, offset));
}

SLVAL
sl_regexp_match_capture(sl_vm_t* vm, SLVAL self, SLVAL i)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, i);
    if(index < 0) {
        return vm->lib.nil;
    }
    int start = sl_string_index_for_byte_offset(vm, match->match_string, match->captures[index]);
    int end = sl_string_index_for_byte_offset(vm, match->match_string, match->captures[index + 1]);
    SLVAL off_len[] = { sl_make_int(vm, start), sl_make_int(vm, end - start) };
    return sl_make_array(vm, 2, off_len);
}

static SLVAL
sl_regexp_match_length(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, get_regexp_match(vm, self)->capture_count);
}

SLVAL
sl_regexp_match_before(sl_vm_t* vm, SLVAL self)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, sl_make_int(vm, 0));
    sl_string_t* str = (sl_string_t*)sl_get_ptr(match->match_string);
    return sl_make_string(vm, str->buff, match->captures[index]);
}

SLVAL
sl_regexp_match_after(sl_vm_t* vm, SLVAL self)
{
    sl_regexp_match_t* match = get_regexp_match(vm, self);
    int index = cap_index(vm, self, sl_make_int(vm, 0));
    sl_string_t* str = (sl_string_t*)sl_get_ptr(match->match_string);
    return sl_make_string(vm, str->buff + match->captures[index + 1], str->buff_len - match->captures[index + 1]);
}

void
sl_init_regexp(sl_vm_t* vm)
{
    vm->lib.Regexp = sl_define_class(vm, "Regexp", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Regexp, allocate_regexp);
    vm->lib.Regexp_Match = sl_define_class3(vm, sl_intern(vm, "Match"), vm->lib.Object, vm->lib.Regexp);
    sl_class_set_allocator(vm, vm->lib.Regexp_Match, allocate_regexp_match);

    sl_define_method(vm, vm->lib.Regexp, "init", -2, sl_regexp_init);
    /*
    sl_define_method(vm, vm->lib.Regexp, "compile", 0, sl_regexp_compile);
    */
    sl_define_method(vm, vm->lib.Regexp, "match", -2, sl_regexp_match);
    sl_define_method(vm, vm->lib.Regexp, "~", 1, sl_regexp_is_match);
    sl_define_method(vm, vm->lib.Regexp, "source", 0, sl_regexp_source);
    sl_define_method(vm, vm->lib.Regexp, "options", 0, sl_regexp_options);
    sl_define_method(vm, vm->lib.Regexp, "==", 1, sl_regexp_eq);

    sl_class_set_const(vm, vm->lib.Regexp, "CASELESS", sl_make_int(vm, PCRE_CASELESS));
    sl_class_set_const(vm, vm->lib.Regexp, "EXTENDED", sl_make_int(vm, PCRE_EXTENDED));
    sl_class_set_const(vm, vm->lib.Regexp, "PCRE_VERSION", sl_make_cstring(vm, pcre_version()));

    sl_define_method(vm, vm->lib.Regexp_Match, "regexp", 0, sl_regexp_match_regexp);
    sl_define_method(vm, vm->lib.Regexp_Match, "[]", 1, sl_regexp_match_index);
    sl_define_method(vm, vm->lib.Regexp_Match, "byte_offset", 1, sl_regexp_match_byte_offset);
    sl_define_method(vm, vm->lib.Regexp_Match, "offset", 1, sl_regexp_match_offset);
    sl_define_method(vm, vm->lib.Regexp_Match, "capture", 1, sl_regexp_match_capture);
    sl_define_method(vm, vm->lib.Regexp_Match, "length", 0, sl_regexp_match_length);
    sl_define_method(vm, vm->lib.Regexp_Match, "before", 0, sl_regexp_match_before);
    sl_define_method(vm, vm->lib.Regexp_Match, "after", 0, sl_regexp_match_after);
}
