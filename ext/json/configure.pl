check_lib "yajl", <<C;
    #include <yajl/yajl_parse.h>
    
    int main() {
        yajl_alloc(NULL, NULL, NULL);
    }
C
