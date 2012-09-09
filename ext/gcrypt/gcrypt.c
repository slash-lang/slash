#include <gcrypt.h>
#include <stdio.h>
#include <alloca.h>
#include <slash.h>

static int cGCrypt;
static int cGCrypt_Algorithm;

typedef struct {
    sl_object_t base;
    SLVAL name;
    int algo;
}
gcrypt_algorithm_t;

static sl_object_t*
allocate_gcrypt_algorithm(sl_vm_t* vm)
{
    gcrypt_algorithm_t* algo = sl_alloc(vm->arena, sizeof(gcrypt_algorithm_t));
    algo->algo = 0;
    algo->name = sl_make_cstring(vm, "(Invalid)");
    return (sl_object_t*)algo;
}

static gcrypt_algorithm_t*
get_algo(sl_vm_t* vm, SLVAL obj)
{
    SLVAL klass = sl_vm_store_get(vm, &cGCrypt_Algorithm);
    gcrypt_algorithm_t* ptr;
    sl_expect(vm, obj, klass);
    ptr = (gcrypt_algorithm_t*)sl_get_ptr(obj);
    return ptr;
}

static gcrypt_algorithm_t*
get_algo_check(sl_vm_t* vm, SLVAL obj)
{
    gcrypt_algorithm_t* ptr = get_algo(vm, obj);
    if(ptr->algo == 0) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid GCrypt::Algorithm");
    }
    return ptr;
}

static void
make_algorithm_object(sl_vm_t* vm, SLVAL GCrypt, SLVAL Algorithm, char* name, int algo)
{
    SLVAL obj = sl_allocate(vm, Algorithm);
    gcrypt_algorithm_t* ptr = (gcrypt_algorithm_t*)sl_get_ptr(obj);
    ptr->name = sl_make_cstring(vm, name);
    ptr->algo = algo;
    sl_class_set_const2(vm, GCrypt, ptr->name, obj);
}

static SLVAL
sl_gcrypt_algorithm_hex_digest(sl_vm_t* vm, SLVAL self, SLVAL strv)
{
    size_t i;
    sl_string_t* str = (sl_string_t*)sl_get_ptr(sl_expect(vm, strv, vm->lib.String));
    gcrypt_algorithm_t* algo = get_algo_check(vm, self);
    size_t digest_len = gcry_md_get_algo_dlen(algo->algo);
    char* digest = alloca(digest_len);
    char* hex_digest = alloca(digest_len * 2);
    gcry_md_hash_buffer(algo->algo, digest, str->buff, str->buff_len);
    for(i = 0; i < digest_len; i++) {
        sprintf(hex_digest + 2 * i, "%02x", (uint8_t)digest[i]);
    }
    return sl_make_string(vm, (uint8_t*)hex_digest, digest_len * 2);
}

static SLVAL
sl_gcrypt_algorithm_inspect(sl_vm_t* vm, SLVAL self)
{
    gcrypt_algorithm_t* algo = get_algo(vm, self);
    SLVAL str = sl_make_cstring(vm, "#<GCrypt::Algorithm: ");
    str = sl_string_concat(vm, str, algo->name);
    str = sl_string_concat(vm, str, sl_make_cstring(vm, ">"));
    return str;
}

void
sl_init_ext_gcrypt(sl_vm_t* vm)
{
    SLVAL GCrypt = sl_define_class(vm, "GCrypt", vm->lib.Object);
    SLVAL Algorithm = sl_define_class3(vm, sl_make_cstring(vm, "Algorithm"), vm->lib.Object, GCrypt);
    sl_vm_store_put(vm, &cGCrypt, GCrypt);
    sl_vm_store_put(vm, &cGCrypt_Algorithm, Algorithm);
    sl_class_set_allocator(vm, Algorithm, allocate_gcrypt_algorithm);
    
    sl_define_method(vm, Algorithm, "hex_digest", 1, sl_gcrypt_algorithm_hex_digest);
    sl_define_method(vm, Algorithm, "to_s", 0, sl_gcrypt_algorithm_inspect);
    sl_define_method(vm, Algorithm, "inspect", 0, sl_gcrypt_algorithm_inspect);
    
    #define MAKE_ALGO(name) make_algorithm_object(vm, GCrypt, Algorithm, #name, GCRY_MD_##name)
    MAKE_ALGO(MD5);
    MAKE_ALGO(SHA1);
    MAKE_ALGO(SHA224);
    MAKE_ALGO(SHA256);
    MAKE_ALGO(SHA384);
    MAKE_ALGO(SHA512);
    MAKE_ALGO(WHIRLPOOL);
}
