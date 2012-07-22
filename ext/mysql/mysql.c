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

typedef struct {
    sl_object_t base;
    int valid;
    mysql_t* mysql;
    MYSQL_STMT* stmt;
}
stmt_t;

static int cMySQL;
static int cMySQL_Error;
static int cMySQL_Statement;

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

static void
free_mysql_statement(stmt_t* stmt)
{
    mysql_stmt_close(stmt->stmt);
}

static sl_object_t*
allocate_mysql_statement()
{
    sl_object_t* stmt = GC_MALLOC(sizeof(stmt_t));
    GC_register_finalizer(stmt, (void(*)(void*,void*))free_mysql_statement, NULL, NULL, NULL);
    return stmt;
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

static stmt_t*
get_stmt(sl_vm_t* vm, SLVAL obj)
{
    stmt_t* stmt;
    sl_expect(vm, obj, sl_vm_store_get(vm, &cMySQL_Statement));
    stmt = (stmt_t*)sl_get_ptr(obj);
    if(!stmt->valid) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Invalid MySQL::Statement instance");
    }
    return stmt;
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
    
    mysql->valid = 1;
    
    return self;
}

static SLVAL
sl_mysql_use(sl_vm_t* vm, SLVAL self, SLVAL db)
{
    mysql_t* mysql = (mysql_t*)sl_get_ptr(self);
    mysql_select_db(&mysql->mysql, sl_to_cstr(vm, sl_expect(vm, db, vm->lib.String)));
    sl_mysql_check_error(vm, &mysql->mysql);
    return vm->lib._true;
}

static SLVAL
sl_mysql_prepare(sl_vm_t* vm, SLVAL self, SLVAL query)
{
    SLVAL argv[2];
    argv[0] = self;
    argv[1] = query;
    return sl_new(vm, sl_vm_store_get(vm, &cMySQL_Statement), 2, argv);
}

static SLVAL
sl_mysql_statement_init(sl_vm_t* vm, SLVAL self, SLVAL mysql, SLVAL query)
{
    stmt_t* stmt = (stmt_t*)sl_get_ptr(self);
    sl_string_t* q;
    stmt->mysql = get_mysql(vm, mysql);
    stmt->stmt = mysql_stmt_init(&stmt->mysql->mysql);
    sl_mysql_check_error(vm, &stmt->mysql->mysql);
    q = (sl_string_t*)sl_get_ptr(sl_expect(vm, query, vm->lib.String));
    mysql_stmt_prepare(stmt->stmt, (char*)q->buff, q->buff_len);
    sl_mysql_check_error(vm, &stmt->mysql->mysql);
    stmt->valid = 1;
    return self;
}

static SLVAL
sl_mysql_statement_execute(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    size_t i, j, len;
    stmt_t* stmt = get_stmt(vm, self);
    sl_string_t* str;
    SLVAL* results;
    SLVAL* cells;
    size_t num_rows, field_count;
    MYSQL_BIND* bind = alloca(sizeof(MYSQL_BIND) * argc);
    MYSQL_BIND out_bind;
    memset(bind, 0, sizeof(MYSQL_BIND) * argc);
    for(i = 0; i < argc; i++) {
        str = (sl_string_t*)sl_get_ptr(sl_to_s(vm, argv[i]));
        if(sl_get_primitive_type(argv[i]) == SL_T_NIL) {
            bind[i].buffer_type = MYSQL_TYPE_NULL;
        } else {
            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = str->buff;
            bind[i].buffer_length = str->buff_len;
        }
    }
    if(mysql_stmt_bind_param(stmt->stmt, bind)) {
        sl_mysql_check_error(vm, &stmt->mysql->mysql);
    }
    if(mysql_stmt_execute(stmt->stmt)) {
        sl_mysql_check_error(vm, &stmt->mysql->mysql);
    }
    if(mysql_stmt_store_result(stmt->stmt)) {
        sl_mysql_check_error(vm, &stmt->mysql->mysql);
    }
    num_rows = mysql_stmt_num_rows(stmt->stmt);
    field_count = mysql_stmt_field_count(stmt->stmt);
    results = GC_MALLOC(sizeof(SLVAL) * num_rows);
    for(i = 0; i < num_rows; i++) {
        cells = GC_MALLOC(sizeof(SLVAL) * field_count);
        mysql_stmt_fetch(stmt->stmt);
        for(j = 0; j < field_count; j++) {
            memset(&out_bind, 0, sizeof(out_bind));
            out_bind.buffer_type = MYSQL_TYPE_STRING;
            out_bind.buffer = GC_MALLOC(65536);
            out_bind.buffer_length = 65536;
            out_bind.length = &len;
            if(mysql_stmt_fetch_column(stmt->stmt, &out_bind, j, 0)) {
                sl_mysql_check_error(vm, &stmt->mysql->mysql);
            }
            cells[j] = sl_make_string(vm, out_bind.buffer, len);
        }
        results[i] = sl_make_array(vm, field_count, cells);
    }
    return sl_make_array(vm, num_rows, results);
}

void
sl_init_ext_mysql(sl_vm_t* vm)
{
    SLVAL MySQL, MySQL_Statement, MySQL_Error;
    MySQL = sl_define_class(vm, "MySQL", vm->lib.Object);
    sl_class_set_allocator(vm, MySQL, allocate_mysql);
    sl_define_method(vm, MySQL, "init", 3, sl_mysql_init);
    sl_define_method(vm, MySQL, "use", 1, sl_mysql_use);
    sl_define_method(vm, MySQL, "prepare", 1, sl_mysql_prepare);
    
    MySQL_Error = sl_define_class3(vm, sl_make_cstring(vm, "Error"), vm->lib.Error, MySQL);
    
    MySQL_Statement = sl_define_class3(vm, sl_make_cstring(vm, "Statement"), vm->lib.Error, MySQL);
    sl_class_set_allocator(vm, MySQL_Statement, allocate_mysql_statement);
    sl_define_method(vm, MySQL_Statement, "init", 2, sl_mysql_statement_init);
    sl_define_method(vm, MySQL_Statement, "execute", -1, sl_mysql_statement_execute);
    
    sl_vm_store_put(vm, &cMySQL, MySQL);
    sl_vm_store_put(vm, &cMySQL_Error, MySQL_Error);
    sl_vm_store_put(vm, &cMySQL_Statement, MySQL_Statement);
}