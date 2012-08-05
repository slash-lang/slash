#include <pcre.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "slash.h"

typedef struct {
    char* re;
    pcre* pcre;
    char* replace;
}
inflection_t;

typedef struct {
    char* singular;
    char* plural;
}
irregular_plural_t;

static irregular_plural_t irregulars[] = {
    { "child",  "children" },
    { "goose",  "geese" },
    { "man",    "men" },
    { "person", "people" },
    { "sex",    "sexes" },
    { "leaf",   "leaves" },
    { "mouse",  "mice" },
    { "quiz",   "quizzes" },
    { "ox",     "oxen" },
    { "foot",   "feet" },
    { "tooth",  "teeth" },
    { "louse",  "lice" }
};

static char* uncountables[] = {
    "equipment",
    "furniture",
    "information",
    "rice",
    "money",
    "species",
    "series",
    "fish",
    "sheep",
    "jeans",
    "police",
    "music",
    "news",
    "advice",
    "luggage",
    "sugar",
    "butter",
    "water",
    "tuna",
    "salmon",
    "trout"
};

#define o(re, plural) { re, NULL, plural }
static inflection_t inflections[] = {
    o("^(ax|test)is$",            "-es"),
    o("(octop|vir)us$",           "-i"),
    o("([ti])um$",                "-a"),
    o("([ti])a$",                 "-a"),
    o("([^aeiouy]|qu)y$",         "-ies"),
    o("^(matr|vert|ind)(ix|ex)$", "-ices"),
    o("(x|ch|ss|sh)$",            "-es"),
    o("sis$",                     "ses"),
    o("s$",                       "ses")
};
#undef o

static SLVAL
string_pluralize(sl_vm_t* vm, SLVAL self)
{
    char* str = sl_to_cstr(vm, self);
    size_t len = strlen(str);
    char* lower = sl_alloc_buffer(vm->arena, len + 1);
    char* ret = NULL;
    size_t i;
    int caps[30], rc;
    SLVAL retn;
    for(i = 0; i < len; i++) {
        lower[i] = tolower(str[i]);
    }
    for(i = 0; i < sizeof(irregulars)/sizeof(*irregulars); i++) {
        if(strcmp(irregulars[i].singular, lower) == 0) {
            ret = sl_alloc_buffer(vm->arena, strlen(irregulars[i].plural) + 1);
            ret[0] = str[0];
            strcpy(ret + 1, irregulars[i].plural + 1);
            return sl_make_cstring(vm, ret);
        }
    }
    for(i = 0; i < sizeof(uncountables)/sizeof(*uncountables); i++) {
        if(strcmp(uncountables[i], lower) == 0) {
            return self;
        }
    }
    for(i = 0; i < sizeof(inflections)/sizeof(*inflections); i++) {
        rc = pcre_exec(inflections[i].pcre, NULL, str, len, 0, 0, caps, 30);
        if(rc < 0) {
            continue;
        }
        retn = sl_make_string(vm, (uint8_t*)str, caps[0]);
        if(inflections[i].replace[0] == '-') {
            retn = sl_string_concat(vm, retn, sl_make_string(vm, (uint8_t*)(str + caps[2]), caps[3] - caps[2]));
            retn = sl_string_concat(vm, retn, sl_make_cstring(vm, inflections[i].replace + 1));
        } else {
            retn = sl_string_concat(vm, retn, sl_make_cstring(vm, inflections[i].replace));
        }
        return retn;
    }
    return sl_string_concat(vm, self, sl_make_cstring(vm, "s"));
}

void
sl_ext_inflect_pluralize_init(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.String, "pluralize", 0, string_pluralize);
}

void
sl_ext_inflect_pluralize_static_init()
{
    size_t i;
    const char* err;
    int erroffset;
    for(i = 0; i < sizeof(inflections)/sizeof(*inflections); i++) {
        err = NULL;
        inflections[i].pcre = pcre_compile(inflections[i].re,
            PCRE_DOLLAR_ENDONLY | PCRE_UTF8 | PCRE_CASELESS, &err, &erroffset, NULL);
        if(err) {
            fprintf(stderr, "[ext/inflect] Could not compile regex /%s/: %s\n", inflections[i].re, err);
        }
    }
}