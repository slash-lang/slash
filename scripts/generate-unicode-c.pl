use strict;

# not distributed with Slash, download from: http://www.unicode.org/Public/3.2-Update/CaseFolding-3.2.0.txt
open my $case_folding, "src/CaseFolding-3.2.0.txt";

open my $output, ">src/unicode.c";

print $output <<C;
#include <slash.h>
#include <stddef.h>

static struct {
    uint32_t upper;
    uint32_t lower;
}
mappings[] = {
C

while(<$case_folding>) {
    next unless /^([A-F0-9]+); [CS]; ([A-F0-9]+); # (.*)$/;
    print $output "    { 0x$1, 0x$2 }, // $3\n";
}

print $output <<C;
    { 0, 0 }
};

uint32_t
sl_unicode_tolower(uint32_t c)
{
    int left = 0;
    int right = (sizeof(mappings) / sizeof(mappings[0])) - 2;
    while(left <= right) {
        int mid = (left + right) / 2;
        if(mappings[mid].upper == c) {
            return mappings[mid].lower;
        }
        if(mappings[mid].upper < c) {
            left = mid + 1;
        }
        if(mappings[mid].upper > c) {
            right = mid - 1;
        }
    }
    return c;
}

uint32_t
sl_unicode_toupper(uint32_t c)
{
    /* we can't binary search toupper because the mappings are sorted in order
       of the capital letters, not the lower case letters */
    for(int i = 0; i < (sizeof(mappings) / sizeof(mappings[0])) - 1; i++) {
        if(mappings[i].lower == c) {
            return mappings[i].upper;
        }
    }
    return c;
}

C
