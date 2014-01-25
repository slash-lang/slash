#include <curl/curl.h>
#include <slash.h>

typedef struct {
    sl_object_t base;
    CURL* curl;
    /* we need to protect pointers we pass into libcurl, so we'll use an array
       to stick data in so it doesn't get GC'd until this object does.
       NOTE: it is not safe to use the pointers in this array. they are not
       guaranteed to be proper slash objects */
    SLVAL gc_protect_array;
}
curl_easy_t;

static int cCurl_Easy;
static int cCurl_Error;

static void
curl_easy_free(void* easy)
{
    curl_easy_cleanup(((curl_easy_t*)easy)->curl);
}

static sl_gc_shape_t
curl_shape = {
    .mark     = sl_gc_conservative_mark,
    .finalize = curl_easy_free,
};

static sl_object_t*
curl_easy_alloc(sl_vm_t* vm)
{
    curl_easy_t* c = sl_alloc2(vm->arena, &curl_shape, sizeof(curl_easy_t));
    c->curl = curl_easy_init();
    c->gc_protect_array = sl_make_array(vm, 0, NULL);
    /* set some good defaults */
    curl_easy_setopt(c->curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(c->curl, CURLOPT_NOSIGNAL, 1);
    return (void*)c;
}

static curl_easy_t*
get_curl_easy(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->store[cCurl_Easy]);
    curl_easy_t* curl = (void*)sl_get_ptr(obj);
    if(!curl->curl) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid operation on uninitialized Curl::Easy");
    }
    return curl;
}

static void
curl_easy_check_error(sl_vm_t* vm, CURLcode code)
{
    if(code != CURLE_OK) {
        SLVAL Curl_Error = vm->store[cCurl_Error];
        sl_throw_message2(vm, Curl_Error, curl_easy_strerror(code));
    }
}

static void
sl_curlopts_init(sl_vm_t* vm, SLVAL Curl);

static SLVAL
sl_curl_easy_setopt(sl_vm_t* vm, SLVAL self, SLVAL vopt, SLVAL argument);

static SLVAL
sl_curl_easy_perform(sl_vm_t* vm, SLVAL self)
{
    curl_easy_t* curl = get_curl_easy(vm, self);
    curl_easy_check_error(vm, curl_easy_perform(curl->curl));
    return vm->lib._true;
}

void
sl_init_ext_curl(sl_vm_t* vm)
{
    SLVAL Curl = sl_define_class(vm, "Curl", vm->lib.Object);
    SLVAL Curl_Error = sl_define_class3(vm, sl_intern(vm, "Error"), vm->lib.Error, Curl);
    sl_class_set_const(vm, Curl, "VERSION", sl_make_cstring(vm, curl_version()));
    SLVAL Curl_Easy_ = sl_define_class3(vm, sl_intern(vm, "Easy"), vm->lib.Object, Curl);
    sl_class_set_allocator(vm, Curl_Easy_, curl_easy_alloc);

    sl_define_method(vm, Curl_Easy_, "set_opt", 2, sl_curl_easy_setopt);
    sl_define_method(vm, Curl_Easy_, "perform", 0, sl_curl_easy_perform);

    vm->store[cCurl_Error] = Curl_Error;
    vm->store[cCurl_Easy] = Curl_Easy_;

    sl_curlopts_init(vm, Curl);
}

void
sl_static_init_ext_curl()
{
    cCurl_Error = sl_vm_store_register_slot();
    cCurl_Easy = sl_vm_store_register_slot();

    curl_global_init(CURL_GLOBAL_DEFAULT);
}

#include "curlopts_defn.inc"

#include "curlopts_impl.inc"
