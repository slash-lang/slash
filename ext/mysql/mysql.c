#include <slash.h>
#include <mysql.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    int valid;
    MYSQL mysql;
}
mysql_t;

typedef struct {
    mysql_t* mysql;
    MYSQL_STMT* stmt;
    MYSQL_BIND* bind;
}
mysql_stmt_t;

static int cMySQL;
static int cMySQL_Statement;
static int cMySQL_Error;

void
sl_static_init_ext_mysql()
{
    mysql_library_init(0, NULL, NULL);

    cMySQL = sl_vm_store_register_slot();
    cMySQL_Statement = sl_vm_store_register_slot();
    cMySQL_Error = sl_vm_store_register_slot();
}

static void
free_mysql(sl_vm_t* vm, void* ptr)
{
    mysql_t* mysql = ptr;
    if(mysql->valid) {
        mysql_close(&mysql->mysql);
    }
    free(mysql);
    (void)vm;
}

static sl_data_type_t
mysql_data_type = {
    .name = "MySQL",
    .free = free_mysql,
};

static sl_object_t*
allocate_mysql(sl_vm_t* vm)
{
    mysql_t* mysql = calloc(1, sizeof(*mysql));
    return sl_make_data(vm, vm->store[cMySQL], &mysql_data_type, mysql);
}

static void
free_mysql_stmt(sl_vm_t* vm, void* ptr)
{
    mysql_stmt_t* stmt = ptr;
    if(stmt->stmt) {
        mysql_stmt_close(stmt->stmt);
    }
    free(stmt);
    (void)vm;
}

static sl_data_type_t
mysql_stmt_data_type = {
    .name = "MySQL::Statement",
    .free = free_mysql_stmt,
};

static sl_object_t*
allocate_mysql_stmt(sl_vm_t* vm)
{
    mysql_stmt_t* stmt = calloc(1, sizeof(*stmt));
    return sl_make_data(vm, vm->store[cMySQL_Statement], &mysql_stmt_data_type, stmt);
}

static mysql_t*
get_mysql(sl_vm_t* vm, SLVAL obj)
{
    mysql_t* mysql = sl_data_get_ptr(vm, &mysql_data_type, obj);
    if(!mysql->valid) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid MySQL instance");
    }
    return mysql;
}

static mysql_stmt_t*
get_mysql_stmt(sl_vm_t* vm, SLVAL obj)
{
    mysql_stmt_t* stmt = sl_data_get_ptr(vm, &mysql_stmt_data_type, obj);
    if(!stmt->stmt) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid operation on uninitialized MySQL::Statement instance");
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
    sl_throw_message2(vm, vm->store[cMySQL_Error], (char*)cerr);
}

static void
sl_mysql_stmt_check_error(sl_vm_t* vm, MYSQL_STMT* stmt)
{
    const char* cerr = mysql_stmt_error(stmt);
    if(!cerr[0]) {
        return;
    }
    sl_throw_message2(vm, vm->store[cMySQL_Error], (char*)cerr);
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

    mysql_t* mysql = sl_data_get_ptr(vm, &mysql_data_type, self);

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
sl_mysql_prepare(sl_vm_t* vm, SLVAL self, SLVAL query)
{
    mysql_t* mysql = get_mysql(vm, self);
    SLVAL stmtv = sl_allocate(vm, vm->store[cMySQL_Statement]);
    mysql_stmt_t* stmt = sl_data_get_ptr(vm, &mysql_stmt_data_type, stmtv);
    stmt->mysql = mysql;
    if(!(stmt->stmt = mysql_stmt_init(&mysql->mysql))) {
        sl_mysql_check_error(vm, &mysql->mysql);
    }

    sl_string_t* str = sl_get_string(vm, query);
    if(mysql_stmt_prepare(stmt->stmt, (char*)str->buff, str->buff_len)) {
        sl_mysql_stmt_check_error(vm, stmt->stmt);
    }

    return stmtv;
}

static SLVAL
sl_mysql_stmt_param_count(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, mysql_stmt_param_count(get_mysql_stmt(vm, self)->stmt));
}

static SLVAL
sl_mysql_stmt_execute(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    mysql_stmt_t* stmt = get_mysql_stmt(vm, self);
    size_t req = mysql_stmt_param_count(stmt->stmt);
    if(argc < req) {
        char buff[100];
        sprintf(buff, "Prepared statement has %lu parameter markers, but only %lu parameters were given", req, argc);
        sl_throw_message2(vm, vm->lib.ArgumentError, buff);
    }

    if(!stmt->bind) {
        stmt->bind = sl_alloc(vm->arena, sizeof(MYSQL_BIND) * req);
    }

    for(size_t i = 0; i < req; i++) {
        stmt->bind[i].buffer_type = MYSQL_TYPE_STRING;
        sl_string_t* str = sl_get_string(vm, sl_to_s(vm, argv[i]));
        stmt->bind[i].buffer = str->buff;
        stmt->bind[i].buffer_length = str->buff_len;
        stmt->bind[i].length = NULL;
        stmt->bind[i].is_null = NULL;
        stmt->bind[i].is_unsigned = 1;
        stmt->bind[i].error = NULL;
    }

    if(mysql_stmt_bind_param(stmt->stmt, stmt->bind)) {
        sl_mysql_stmt_check_error(vm, stmt->stmt);
    }

    if(mysql_stmt_execute(stmt->stmt)) {
        sl_mysql_stmt_check_error(vm, stmt->stmt);
    }

    MYSQL_RES* res = mysql_stmt_result_metadata(stmt->stmt);
    if(!res) {
        /* query did not produce a result set */
        return sl_make_int(vm, mysql_stmt_affected_rows(stmt->stmt));
    }

    int field_count = mysql_stmt_field_count(stmt->stmt);
    MYSQL_FIELD* field;
    SLVAL field_names[field_count];
    enum enum_field_types field_types[field_count];
    size_t field_i = 0;
    while((field = mysql_fetch_field(res))) {
        field_names[field_i] = sl_make_cstring(vm, field->name);
        if(field->type == MYSQL_TYPE_LONG || field->type == MYSQL_TYPE_SHORT || field->type == MYSQL_TYPE_TINY) {
            field_types[field_i] = MYSQL_TYPE_LONG;
        } else {
            field_types[field_i] = MYSQL_TYPE_STRING;
        }
        field_i++;
    }

    MYSQL_BIND output_binds[field_count];
    my_bool output_errors[field_count];
    my_bool output_is_nulls[field_count];
    unsigned long output_lengths[field_count];
    for(int i = 0; i < field_count; i++) {
        output_binds[i].buffer_type = MYSQL_TYPE_STRING;
        output_binds[i].buffer = NULL;
        output_binds[i].buffer_length = 0;
        output_binds[i].length = &output_lengths[i];
        output_binds[i].is_null = &output_is_nulls[i];
        output_binds[i].error = &output_errors[i];
    }
    if(mysql_stmt_bind_result(stmt->stmt, output_binds)) {
        sl_mysql_stmt_check_error(vm, stmt->stmt);
    }

    SLVAL result_rows = sl_make_array(vm, 0, NULL);
    while(1) {
        int code = mysql_stmt_fetch(stmt->stmt);
        if(code == MYSQL_NO_DATA) {
            break;
        }
        if(code == 1) {
            sl_mysql_stmt_check_error(vm, stmt->stmt);
        }
        SLVAL row = sl_make_dict(vm, 0, NULL);
        for(int i = 0; i < field_count; i++) {
            MYSQL_BIND cell;
            cell.length = &output_lengths[i];
            cell.is_null = &output_is_nulls[i];
            cell.error = &output_errors[i];
            cell.buffer_type = field_types[i];
            int buffer_long;
            switch(field_types[i]) {
                case MYSQL_TYPE_LONG:
                    cell.buffer = &buffer_long;
                    cell.buffer_length = sizeof(buffer_long);
                    break;
                default: /* MYSQL_TYPE_STRING */
                    cell.buffer = sl_alloc_buffer(vm->arena, output_lengths[i] + 1);
                    cell.buffer_length = output_lengths[i];
                    break;
            }
            if(mysql_stmt_fetch_column(stmt->stmt, &cell, i, 0)) {
                sl_mysql_stmt_check_error(vm, stmt->stmt);
            }
            switch(field_types[i]) {
                case MYSQL_TYPE_LONG:
                    sl_dict_set(vm, row, field_names[i], sl_make_int(vm, buffer_long));
                    break;
                default: /* MYSQL_TYPE_STRING */
                    sl_dict_set(vm, row, field_names[i], sl_make_string(vm, cell.buffer, output_lengths[i]));
                    break;
            }
        }
        sl_array_push(vm, result_rows, 1, &row);
    }

    return result_rows;
}

static SLVAL
sl_mysql_stmt_insert_id(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, mysql_stmt_insert_id(get_mysql_stmt(vm, self)->stmt));
}

static SLVAL
sl_mysql_raw_query(sl_vm_t* vm, SLVAL self, SLVAL query)
{
    mysql_t* mysql = get_mysql(vm, self);
    sl_string_t* str = sl_get_string(vm, query);

    if(mysql_real_query(&mysql->mysql, (char*)str->buff, str->buff_len)) {
        sl_mysql_check_error(vm, &mysql->mysql);
    }

    MYSQL_RES* result;
    if((result = mysql_store_result(&mysql->mysql))) {
        /* do shit */
        int ncolumns = mysql_num_fields(result);
        int nrows = mysql_num_rows(result);
        SLVAL* rows = sl_alloc(vm->arena, sizeof(SLVAL) * nrows);
        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        for(int i = 0; i < nrows; i++) {
            SLVAL* cells = sl_alloc(vm->arena, sizeof(SLVAL) * ncolumns * 2);
            MYSQL_ROW row = mysql_fetch_row(result);
            size_t* lengths = mysql_fetch_lengths(result);
            for(int j = 0; j < ncolumns; j++) {
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
sl_mysql_query(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    if(argc == 1) {
        return sl_mysql_raw_query(vm, self, argv[0]);
    }
    SLVAL stmt = sl_mysql_prepare(vm, self, argv[0]);
    return sl_mysql_stmt_execute(vm, stmt, argc - 1, argv + 1);
}

static SLVAL
sl_mysql_escape(sl_vm_t* vm, SLVAL self, SLVAL str)
{
    mysql_t* mysql = get_mysql(vm, self);
    sl_string_t* s = sl_get_string(vm, str);
    char* esc = sl_alloc(vm->arena, s->buff_len * 2 + 1);
    size_t esc_len = mysql_real_escape_string(&mysql->mysql, esc, (char*)s->buff, s->buff_len);
    return sl_make_string(vm, (uint8_t*)esc, esc_len);
}

static SLVAL
sl_mysql_insert_id(sl_vm_t* vm, SLVAL self)
{
    mysql_t* mysql = get_mysql(vm, self);
    return sl_make_int(vm, mysql_insert_id(&mysql->mysql));
}

extern char*
sl__ext_mysql_extensions_sl;

void
sl_init_ext_mysql(sl_vm_t* vm)
{
    SLVAL MySQL, MySQL_Error, MySQL_Statement;
    MySQL = sl_define_class(vm, "MySQL", vm->lib.Object);
    sl_class_set_allocator(vm, MySQL, allocate_mysql);
    sl_define_method(vm, MySQL, "init", 3, sl_mysql_init);
    sl_define_method(vm, MySQL, "use", 1, sl_mysql_use);
    sl_define_method(vm, MySQL, "query", -2, sl_mysql_query);
    sl_define_method(vm, MySQL, "prepare", 1, sl_mysql_prepare);
    sl_define_method(vm, MySQL, "escape", 1, sl_mysql_escape);
    sl_define_method(vm, MySQL, "insert_id", 0, sl_mysql_insert_id);

    MySQL_Error = sl_define_class3(vm, sl_intern(vm, "Error"), vm->lib.Error, MySQL);

    MySQL_Statement = sl_define_class3(vm, sl_intern(vm, "Statement"), vm->lib.Object, MySQL);
    sl_class_set_allocator(vm, MySQL_Statement, allocate_mysql_stmt);
    sl_define_method(vm, MySQL_Statement, "param_count", 0, sl_mysql_stmt_param_count);
    sl_define_method(vm, MySQL_Statement, "execute", -1, sl_mysql_stmt_execute);
    sl_define_method(vm, MySQL_Statement, "insert_id", 0, sl_mysql_stmt_insert_id);

    vm->store[cMySQL] = MySQL;
    vm->store[cMySQL_Error] = MySQL_Error;
    vm->store[cMySQL_Statement] = MySQL_Statement;

    sl_do_string(vm, (uint8_t*)sl__ext_mysql_extensions_sl, strlen(sl__ext_mysql_extensions_sl), "ext/mysql/extensions.sl", 0);
}
