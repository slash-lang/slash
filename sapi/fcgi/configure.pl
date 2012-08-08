check_lib "fcgi", <<C;
    #include <fcgi_config.h>
    #include <fcgiapp.h>
    
    int main()
    {
        FCGX_Init();
    }
C
