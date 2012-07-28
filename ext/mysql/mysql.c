#include "slash.h"
#include <mysql.h>
#include <gc.h>
#include <alloca.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    sl_object_t base;
    int valid;
    MYSQL mysql;
}
mysql_t;

static int cMySQL;
static int cMySQL_Error;

void
sl_static_init_ext_mysql()
{
    mysql_library_init(0, NULL, NULL);
}

static void
free_mysql(mysql_t* mysql)
{
    mysql_close(&mysql->mysql);
}

static sl_object_t*
allocate_mysql()
{
    mysql_t* mysql = GC_MALLOC(sizeof(mysql_t));
    mysql_init(&mysql->mysql);
    GC_register_finalizer(mysql, (void(*)(void*,void*))free_mysql, NULL, NULL, NULL);
    return (sl_object_t*)mysql;
}

static mysql_t*
get_mysql(sl_vm_t* vm, SLVAL obj)
{
    mysql_t* mysql;
    sl_expect(vm, obj, sl_vm_store_get(vm, &cMySQL));
    mysql = (mysql_t*)sl_get_ptr(obj);
    if(!mysql->valid) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Invalid MySQL instance");
    }
    return mysql;
}

static void
sl_mysql_check_error(sl_vm_t* vm, MYSQL* mysql)
{
    const char* cerr = mysql_error(mysql);
    if(!cerr[0]) {
        return;
    }
    sl_throw_message2(vm, sl_vm_store_get(vm, &cMySQL_Error), (char*)cerr);
}

static SLVAL
sl_mysql_init(sl_vm_t* vm, SLVAL self, SLVAL host, SLVAL user, SLVAL password)
{
    char *host_s = NULL,
         *user_s = NULL,
         *password_s = NULL,
         *db_s = NULL;
    int  port_i = 3306,
         flag_i = CLIENT_IGNORE_SIGPIPE | CLIENT_MULTI_STATEMENTS;
         
    mysql_t* mysql = (mysql_t*)sl_get_ptr(self);
    
    if(sl_is_truthy(host)) {
        host_s = sl_to_cstr(vm, sl_expect(vm, host, vm->lib.String));
    }
    if(sl_is_truthy(user)) {
        user_s = sl_to_cstr(vm, sl_expect(vm, user, vm->lib.String));
    }
    if(sl_is_truthy(password)) {
        password_s = sl_to_cstr(vm, sl_expect(vm, password, vm->lib.String));
    }
    
    if(!mysql_real_connect(&mysql->mysql, host_s, user_s, password_s, db_s, port_i, NULL, flag_i)) {
        sl_mysql_check_error(vm, &mysql->mysql);
    }
    
    if(!mysql_set_character_set(&mysql->mysql, "utf8")) {
        sl_mysql_check_error(vm, &mysql->mysql);
    }
    
    mysql->valid = 1;
    
    return self;
}

static SLVAL
sl_mysql_use(sl_vm_t* vm, SLVAL self, SLVAL db)
{
    mysql_t* mysql = get_mysql(vm, self);
    mysql_select_db(&mysql->mysql, sl_to_cstr(vm, sl_expect(vm, db, vm->lib.String)));
    sl_mysql_check_error(vm, &mysql->mysql);
    return vm->lib._true;
}

static SLVAL
sl_mysql_query(sl_vm_t* vm, SLVAL self, SLVAL query)
{
    mysql_t* mysql = get_mysql(vm, self);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(sl_expect(vm, query, vm->lib.String));
    MYSQL_RES* result;
    MYSQL_FIELD* fields;
    MYSQL_ROW row;
    size_t ncolumns, nrows;
    SLVAL* rows;
    SLVAL* cells;
    size_t i, j;
    size_t* lengths;
    if(mysql_real_query(&mysql->mysql, (char*)str->buff, str->buff_len)) {
        sl_mysql_check_error(vm, &mysql->mysql);
    }
    if((result = mysql_store_result(&mysql->mysql))) {
        /* do shit */
        ncolumns = mysql_num_fields(result);
        nrows = mysql_num_rows(result);
        rows = GC_MALLOC(sizeof(SLVAL) * nrows);
        fields = mysql_fetch_fields(result);
        for(i = 0; i < nrows; i++) {
            cells = GC_MALLOC(sizeof(SLVAL) * ncolumns * 2);
            row = mysql_fetch_row(result);
            lengths = mysql_fetch_lengths(result);
            for(j = 0; j < ncolumns; j++) {
                cells[j * 2] = sl_make_cstring(vm, fields[j].name);
                if(row[j]) {
                    cells[j * 2 + 1] = sl_make_string(vm, (uint8_t*)row[j], lengths[j]);
                } else {
                    cells[j * 2 + 1] = vm->lib.nil;
                }
            }
            rows[i] = sl_make_dict(vm, ncolumns, cells);
        }
        mysql_free_result(result);
        return sl_make_array(vm, nrows, rows);
    } else {
        if(mysql_field_count(&mysql->mysql) != 0) {
            sl_mysql_check_error(vm, &mysql->mysql);
        }
        return sl_make_int(vm, mysql_affected_rows(&mysql->mysql));
    }
}

static SLVAL
sl_mysql_escape(sl_vm_t* vm, SLVAL self, SLVAL str)
{
    mysql_t* mysql = get_mysql(vm, self);
    sl_string_t* s = (sl_string_t*)sl_get_ptr(sl_expect(vm, str, vm->lib.String));
    char* esc = GC_MALLOC(s->buff_len * 2 + 1);
    size_t esc_len = mysql_real_escape_string(&mysql->mysql, esc, (char*)s->buff, s->buff_len);
    return sl_make_string(vm, (uint8_t*)esc, esc_len);
}

static SLVAL
sl_mysql_insert_id(sl_vm_t* vm, SLVAL self)
{
    mysql_t* mysql = get_mysql(vm, self);
    return sl_make_int(vm, mysql_insert_id(&mysql->mysql));
}

void
sl_init_ext_mysql(sl_vm_t* vm)
{
    SLVAL MySQL, MySQL_Error;
    MySQL = sl_define_class(vm, "MySQL", vm->lib.Object);
    sl_class_set_allocator(vm, MySQL, allocate_mysql);
    sl_define_method(vm, MySQL, "init", 3, sl_mysql_init);
    sl_define_method(vm, MySQL, "use", 1, sl_mysql_use);
    sl_define_method(vm, MySQL, "query", 1, sl_mysql_query);
    sl_define_method(vm, MySQL, "escape", 1, sl_mysql_escape);
    sl_define_method(vm, MySQL, "insert_id", 0, sl_mysql_insert_id);
    
    MySQL_Error = sl_define_class3(vm, sl_make_cstring(vm, "Error"), vm->lib.Error, MySQL);
    
    sl_vm_store_put(vm, &cMySQL, MySQL);
    sl_vm_store_put(vm, &cMySQL_Error, MySQL_Error);
}
