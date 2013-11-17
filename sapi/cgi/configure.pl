check_lib "fcgi", <<C;
    #include <fcgiapp.h>

    int main() {
        FCGX_IsCGI();
    }
C
