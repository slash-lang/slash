check_lib "mysqlclient", <<C;
    #include <mysql.h>
    
    int main() {
        MYSQL mysql;
        mysql_init(&mysql);
    }
C

needs_static_init;