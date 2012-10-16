check_lib "markdown", <<C;
    #include <mkdio.h>
    #include <string.h>
    
    int main() {
        char* mkd = "# Heading!";
        MMIOT* mmiot = mkd_string(mkd, strlen(mkd), 0);
    }
C
