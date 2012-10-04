check_lib "sass", <<C;
    #include <sass_interface.h>
    
    int main() {
        sass_new_context();
    }
C
