check_lib "yajl_s", <<C;
    #include <yajl/yajl_parse.h>
    
    int main() {
        yajl_alloc(0, 0, 0);
    }
C
