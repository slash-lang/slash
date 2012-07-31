check_lib "yajl_s", <<C;
    #include <yajl/yajl_parse.h>
    
    int main() {
        char buff[100];
        yajl_parse_integer(buff, 123456789);
    }
C
