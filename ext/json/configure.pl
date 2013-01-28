check_lib "yajl", <<C;
    #include <yajl/yajl_parse.h>
    
    int main() {
        yajl_alloc(0, 0, 0);
    }
C
