$src = <<C;
    #include <fcgiapp.h>

    int main() {
        FCGX_IsCGI();
    }
C

check_lib "fcgi", $src, skip_ldflag => 1;
