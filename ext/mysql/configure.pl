if($^O eq "linux") {
    $CFLAGS .= " -I/usr/include/mysql";
    foreach(@lib_dirs) {
        $CFLAGS .= " -I$_/mysql";
    }
}

check_lib "mysqlclient", <<C;
    #include <mysql.h>
    
    int main() {
        MYSQL mysql;
        mysql_init(&mysql);
    }
C

needs_static_init;