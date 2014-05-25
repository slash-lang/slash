#define SL_TEST

#include "cgi.c"

static  SLVAL
cgi_test_get_hashbang_length(sl_vm_t* vm, SLVAL CgiTest, SLVAL str)
{
    sl_string_t* strobj = sl_get_string(vm, str);
    size_t hblen = get_hashbang_length(strobj->buff, strobj->buff_len);

    return sl_make_int(vm, (long)hblen);
}

typedef struct slash_api_test {
    slash_api_base_t base;
    sl_vm_t* vm;
} slash_api_test_t;

static size_t
slash_api_test_write(slash_api_base_t* self, const char * str, size_t n)
{ 
    slash_api_test_t* testSelf = (slash_api_test_t*)self;
    sl_throw_message2(testSelf->vm, testSelf->vm->lib.Error, 
        "API write_out/write_err must not be called at this stage!");
    return 0;
}

static size_t
slash_api_test_read(slash_api_base_t* self, char * str, size_t n)
{
    slash_api_test_t* testSelf = (slash_api_test_t*)self;
    sl_throw_message2(testSelf->vm, testSelf->vm->lib.Error, 
        "API read_in must not be called at this stage!");
    return 0;
}

static SLVAL
cgi_test_kv_list_to_dict(sl_vm_t* vm, sl_request_key_value_list_t* list)
{
    size_t i;
    SLVAL res;
    res = sl_make_dict(vm, 0, NULL);

    for(i = 0; list && i < list->count; ++i) {
        sl_dict_set(vm, res, 
            sl_make_cstring(vm, list->kvs[i].name),
            sl_make_cstring(vm, list->kvs[i].value));
    }

    return res;
}

#define CGI_TEST_DICT_SET_KVP(vm, dict, name, kvp) \
    sl_dict_set(vm, dict, \
        sl_make_cstring(vm, name), \
        kvp && kvp->value ? \
            sl_make_cstring(vm, kvp->value) : vm->lib.nil)

#define CGI_TEST_DICT_SET_CSTR(vm, dict, name, cstr) \
    sl_dict_set(vm, dict, \
        sl_make_cstring(vm, name), \
        cstr ? sl_make_cstring(vm, cstr) : vm->lib.nil)

static SLVAL
cgi_test_request_info_to_slval(sl_vm_t* vm, slash_request_info_t* info)
{
    SLVAL res;

    res = sl_make_dict(vm, 0, NULL);

    CGI_TEST_DICT_SET_KVP(vm, res, "script_name", info->script_name);
    CGI_TEST_DICT_SET_KVP(vm, res, "path_translated", info->path_translated);
    CGI_TEST_DICT_SET_KVP(vm, res, "path_info", info->path_info);
    CGI_TEST_DICT_SET_KVP(vm, res, "script_filename", info->script_filename);
    CGI_TEST_DICT_SET_KVP(vm, res, "content_length", info->content_length);
    CGI_TEST_DICT_SET_KVP(vm, res, "content_type", info->content_type);
    CGI_TEST_DICT_SET_KVP(vm, res, "request_uri", info->request_uri);
    CGI_TEST_DICT_SET_KVP(vm, res, "query_string", info->query_string);
    CGI_TEST_DICT_SET_KVP(vm, res, "remote_addr", info->remote_addr);
    CGI_TEST_DICT_SET_KVP(vm, res, "request_method", info->request_method);

    CGI_TEST_DICT_SET_CSTR(vm, res, "real_uri", info->real_uri);
    CGI_TEST_DICT_SET_CSTR(vm, res, "real_path_info", info->real_path_info);

    CGI_TEST_DICT_SET_CSTR(vm, res, "real_canonical_filename",
        info->real_canonical_filename);

    CGI_TEST_DICT_SET_CSTR(vm, res, "real_canonical_dir",
        info->real_canonical_dir);

    sl_dict_set(vm, res,
        sl_make_cstring(vm, "environment"),
        cgi_test_kv_list_to_dict(vm, info->environment));

    sl_dict_set(vm, res,
        sl_make_cstring(vm, "http_headers"),
        cgi_test_kv_list_to_dict(vm, info->http_headers));

    return res;
}

static SLVAL
cgi_test_load_request_info(sl_vm_t* vm, SLVAL CgiTest, SLVAL api_type, SLVAL env)
{ 
    slash_api_test_t api;
    slash_request_info_t info;
    char ** envp = NULL;
    int type = sl_get_int(sl_expect(vm, api_type, vm->lib.Int));

    if(sl_is_a(vm, env, vm->lib.Dict)) {
        SLVAL* keys;
        size_t length, i;

        keys = sl_dict_keys(vm, env, &length);

        if(length > 0) {
            envp = sl_alloc(vm->arena, (length+1) * sizeof(char*));

            for(i = 0; keys && i < length; ++i) {
                sl_string_t* key;
                sl_string_t* val;
                char* item;

                key = sl_get_string(vm, keys[i]);
                val = sl_get_string(vm, sl_dict_get(vm, env, keys[i]));

                item = sl_alloc(vm->arena, key->buff_len + val->buff_len + 2);

                memcpy(item, key->buff, key->buff_len);
                memcpy(item + key->buff_len + 1, val->buff, val->buff_len);
                item[key->buff_len] = '=';
                item[key->buff_len + val->buff_len + 1] = '\0';

                envp[i] = item;
            }

            envp[length] = NULL;
        }
    } else if(sl_is_a(vm, env, vm->lib.Array)) {
        SLVAL length_val;
        int length, i;
        
        length_val = sl_array_length(vm, env);

        if(!sl_is_a(vm, length_val, vm->lib.Int)) {
            sl_error(vm, vm->lib.TypeError, "Array too big: %X", env);
        }

        length = sl_get_int(length_val);

        if(length > 0) {
            envp = sl_alloc(vm->arena, (length+1) * sizeof(char*));

            for(i = 0; i < length; ++i) {
                char* item;
                sl_string_t* str = sl_get_string(vm, sl_array_get(vm, env, i));
                
                item = sl_alloc(vm->arena, str->buff_len + 1);
                memcpy(item, str->buff, str->buff_len);
                item[str->buff_len] = '\0';

                envp[i] = item;
            }
            envp[length] = NULL;
        }
    } else {
        sl_error(vm, vm->lib.TypeError,
            "Expected %V or %V, instead have %X",
            vm->lib.Array, vm->lib.Dict, env);
    }

    api.vm = vm;
    api.base.type = (slash_api_type_t)type;
    api.base.environ = envp;
    api.base.write_out = slash_api_test_write;
    api.base.write_err = slash_api_test_write;
    api.base.read_in = slash_api_test_read;

    load_request_info(vm, NULL, (slash_api_base_t*)&api, &info);

    return cgi_test_request_info_to_slval(vm, &info);
}

static void register_cgi_test_utils(sl_vm_t* vm)
{
    SLVAL CgiTest = sl_define_class(vm, "CgiTest", vm->lib.Object);

    sl_class_set_const(vm, CgiTest, "SLASH_REQUEST_CGI",
        sl_make_int(vm, SLASH_REQUEST_CGI));

    sl_class_set_const(vm, CgiTest, "SLASH_REQUEST_FCGI",
        sl_make_int(vm, SLASH_REQUEST_FCGI));

    sl_define_singleton_method(vm, CgiTest, "get_hashbang_length", 1,
        cgi_test_get_hashbang_length);

    sl_define_singleton_method(vm, CgiTest, "load_request_info", 2,
        cgi_test_load_request_info);
}
