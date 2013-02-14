check_lib "curl", <<C;
    #include <curl/curl.h>
    
    int main() {
        char* ver = curl_version();
    }
C

needs_static_init;
