check_lib "gcrypt", <<C;
    #include <gcrypt.h>
    
    int main()
    {
        gcry_md_hd_t hd;
        gcry_md_open(&hd, GCRY_MD_SHA1, 0);
    }
C
