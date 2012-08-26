return({
    ['windows'] = {
        ['idirs'] = {
            ['gmp'] = 'win32/slash/gmp-5.0.5/include',
            ['pcre'] = 'win32/slash/pcre-8.31/include',
            ['iconv'] = 'win32/slash/libiconv-1.14/include',
        },

        ['ldirs'] = {
            ['gmp'] = 'win32/slash/gmp-5.0.5/lib',
            ['pcre'] = 'win32/slash/pcre-8.31/lib',
            ['iconv'] = 'win32/slash/libiconv-1.14/lib',
        },

        ['libs'] = {
            'gmp',
            'pcre',
            'iconv',
        },

        ['files'] = {
            'inc/**.h',
            'src/**.c',
        },
    },

    ['macosx'] = {
        ['idirs'] = {
            ['gmp'] = {},
            ['pcre'] = {},
            ['iconv'] = {},
        },

        ['ldirs'] = {
            ['gmp'] = {},
            ['pcre'] = {},
            ['iconv'] = {},
        },

        ['libs'] = {
            'gmp',
            'pcre',
            'iconv',
        },

        ['files'] = {
            'inc/**.h',
            'src/**.c'
        },
    },

    ['linux'] = {
        ['idirs'] = {
            ['gmp'] = {},
            ['pcre'] = {},
            ['iconv'] = {},
        },

        ['ldirs'] = {
            ['gmp'] = {},
            ['pcre'] = {},
            ['iconv'] = {},
        },

        ['libs'] = {
            'gmp',
            'pcre',
            'iconv',
        },

        ['files'] = {
            'inc/**.h',
            'src/**.c',
        },
    },
})