#include "slash.h"
#include <yajl/yajl_parse.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JSON_DEPTH 32
#define JSON_TYPE_DICT 1
#define JSON_TYPE_ARRAY 2

static int cJSON;
static int cJSON_ParseError;

typedef struct {
    size_t max_depth;
    sl_vm_t* vm;
    size_t stack_len;
    size_t stack_cap;
    SLVAL key;
    SLVAL* stack;
    char* type_stack;
    yajl_handle yajl;
}
json_t;

#define JSON_ADD_VALUE(value) do { \
        if(json->stack_len == 0) { \
            json->stack[json->stack_len] = (value); \
        } else { \
            if(json->type_stack[json->stack_len - 1] == JSON_TYPE_DICT) { \
                sl_dict_set(json->vm, json->stack[json->stack_len - 1], json->key, (value)); \
            } else { \
                sl_array_push(json->vm, json->stack[json->stack_len - 1], 1, &(value)); \
            } \
        } \
    } while(0)
    
#define JSON_PUSH_CONTAINER(value, type) do { \
        JSON_ADD_VALUE(value); \
        if(json->stack_len >= json->max_depth) { \
            return 0; \
        } \
        if(json->stack_len >= json->stack_cap) { \
            json->stack_cap *= 2; \
            json->stack = GC_REALLOC(json->stack, sizeof(SLVAL) * json->stack_cap); \
            json->type_stack = GC_REALLOC(json->type_stack, json->stack_cap); \
        } \
        json->type_stack[json->stack_len] = (type); \
        json->stack[json->stack_len++] = (value); \
    } while(0)
    
#define JSON_POP_CONTAINER() json->stack_len--

static int
on_null(void* ctx)
{
    json_t* json = ctx;
    JSON_ADD_VALUE(json->vm->lib.nil);
    return 1;
}

static int
on_boolean(void* ctx, int val)
{
    json_t* json = ctx;
    SLVAL b = val ? json->vm->lib._true : json->vm->lib._false;
    JSON_ADD_VALUE(b);
    return 1;
}

static int
on_integer(void* ctx, long long val)
{
    json_t* json = ctx;
    char buff[256];
    SLVAL i;
    if(val >= SL_MAX_INT || val <= SL_MIN_INT) {
        sprintf(buff, "%lld", val);
        i = sl_make_bignum_s(json->vm, buff);
    } else {
        i = sl_make_int(json->vm, (int)val);
    }
    JSON_ADD_VALUE(i);
    return 1;
}

static int
on_double(void* ctx, double val)
{
    json_t* json = ctx;
    SLVAL f = sl_make_float(json->vm, val);
    JSON_ADD_VALUE(f);
    return 1;
}

static int
on_string(void* ctx, const uint8_t* str, size_t len)
{
    json_t* json = ctx;
    SLVAL s = sl_make_string(json->vm, (uint8_t*)str, len);
    JSON_ADD_VALUE(s);
    return 1;
}

static int
on_start_map(void* ctx)
{
    json_t* json = ctx;
    SLVAL m = sl_make_dict(json->vm, 0, NULL);
    JSON_PUSH_CONTAINER(m, JSON_TYPE_DICT);
    return 1;
}

static int
on_map_key(void* ctx, const uint8_t* str, size_t len)
{
    json_t* json = ctx;
    json->key = sl_make_string(json->vm, (uint8_t*)str, len);
    return 1;
}

static int
on_end_map(void* ctx)
{
    json_t* json = ctx;
    JSON_POP_CONTAINER();
    return 1;
}

static int
on_start_array(void* ctx)
{
    json_t* json = ctx;
    SLVAL a = sl_make_array(json->vm, 0, NULL);
    JSON_PUSH_CONTAINER(a, JSON_TYPE_ARRAY);
    return 1;
}

static int
on_end_array(void* ctx)
{
    json_t* json = ctx;
    JSON_POP_CONTAINER();
    return 1;
}

static yajl_callbacks
callbacks = {
    on_null,
    on_boolean,
    on_integer,
    on_double,
    NULL,
    on_string,
    on_start_map,
    on_map_key,
    on_end_map,
    on_start_array,
    on_end_array
};

static void*
sl_yajl_alloc(void* ctx, size_t sz)
{
    (void)ctx;
    return GC_MALLOC(sz);
}

static void
sl_yajl_free(void* ctx, void* ptr)
{
    (void)ctx;
    GC_FREE(ptr);
}

static void*
sl_yajl_realloc(void* ctx, void* ptr, size_t sz)
{
    (void)ctx;
    return GC_REALLOC(ptr, sz);
}

static yajl_alloc_funcs
alloc_funcs = {
    sl_yajl_alloc,
    sl_yajl_realloc,
    sl_yajl_free,
    NULL
};

static void
sl_json_parse_check_error(sl_vm_t* vm, sl_string_t* str, json_t* json, yajl_status status)
{
    uint8_t* err_str;
    SLVAL err;
    if(status == yajl_status_client_canceled) {
        /* the only reason we'd cancel the parse is if the data structre is too deep */
        yajl_free(json->yajl);
        sl_throw_message2(vm, sl_vm_store_get(vm, &cJSON_ParseError),
            "JSON structure recurses too deep");
    }
    if(status == yajl_status_error) {
        err_str = yajl_get_error(json->yajl, 0, (const uint8_t*)str->buff, (size_t)str->buff_len);
        err = sl_make_cstring(vm, (char*)err_str);
        yajl_free_error(json->yajl, err_str);
        yajl_free(json->yajl);
        sl_throw(vm, sl_make_error2(vm, sl_vm_store_get(vm, &cJSON_ParseError), err));
    }
}

static SLVAL
sl_json_parse(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_string_t* str = (sl_string_t*)sl_get_ptr(argv[0]);
    json_t json;
    json.vm = vm;
    json.max_depth = 32;
    json.stack_len = 0;
    json.stack_cap = 32;
    json.stack = GC_MALLOC(sizeof(SLVAL) * json.stack_cap);
    json.type_stack = GC_MALLOC(json.stack_cap);
    if(argc > 1) {
        json.max_depth = sl_get_int(sl_expect(vm, argv[1], vm->lib.Int));
    }
    
    json.yajl = yajl_alloc(&callbacks, &alloc_funcs, &json);
    sl_json_parse_check_error(vm, str, &json, yajl_parse(json.yajl, str->buff, str->buff_len));
    sl_json_parse_check_error(vm, str, &json, yajl_complete_parse(json.yajl));
    yajl_free(json.yajl);

    /* must've been OK! */
    return json.stack[0];
        
    (void)self;
}

void
sl_init_ext_json(sl_vm_t* vm)
{
    SLVAL JSON = sl_define_class(vm, "JSON", vm->lib.Object);
    SLVAL JSON_ParseError = sl_define_class3(vm, sl_make_cstring(vm, "ParseError"), vm->lib.SyntaxError, JSON);
    sl_define_singleton_method(vm, JSON, "parse", -2, sl_json_parse);
    
    sl_vm_store_put(vm, &cJSON, JSON);
    sl_vm_store_put(vm, &cJSON_ParseError, JSON_ParseError);
}