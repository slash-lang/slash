check_lib "mysqlclient", <<C;
    #include <my_global.h>
    #include <my_sys.h>
    
    int main() {
        my_init();
    }
C

needs_static_init;