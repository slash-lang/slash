<%

class StringTest extends Test {
    def test_length {
        assert_equal(0, "".length);
        assert_equal(5, "hello".length);
        assert_equal(8, "«ταБЬℓσ»".length);
        assert_equal(11, "Justice - †".length);
        assert_equal(11, "Justice - \xe2\x80\xa0".length);
        assert_equal(12, "いろはにほへとちりぬるを".length);
        assert_equal(52, "Γαζέες καὶ μυρτιὲς δὲν θὰ βρῶ πιὰ στὸ χρυσαφὶ ξέφωτο".length);
        assert_equal(32, "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ".length);
    }
    
    def test_concat {
        assert_equal("", "" + "");
        assert_equal("foobar", "foo" + "bar");
        assert_equal("АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ", "АБВГДЕЖЗИЙКЛМНО" + "ПРСТУФХЦЧШЩЪЫЬЭЮЯ");
        assert_equal("helloworld", "hello".concat("world"));
    }
    
    def test_times {
        assert_equal("", "x" * 0);
        assert_equal("xxxxx", "x" * 5);
        assert_equal("\x00\x00\x00\x00\x00\x00", "\x00" * 6);
        assert_equal("†††", "†" * 3);
        assert_throws(TypeError, \{ "x" * 10000000000000000 });
        assert_throws(ArgumentError, \{ "x" * -1 });
    }
    
    def test_to_s {
        assert_equal("", "".to_s);
        assert_equal("hello", "hello".to_s);
    }
    
    def test_to_i {
        assert_equal(123, "123".to_i);
        assert_equal(100000000000000000000000000000000000000, "100000000000000000000000000000000000000".to_i);
        assert_is_a(Bignum, "100000000000000000000000000000000000000".to_i);
        assert_equal(-999, "-999".to_i);
        assert_equal(2, "2.99".to_i);
        assert_equal(7, "     7".to_i);
        assert_equal(0, " not 7".to_i);
    }

    def test_to_f {
        assert_equal(123.0, "123".to_f);
        assert_equal(123.4, "123.4".to_f);
        assert_equal(-0, "-0".to_f);
        assert_equal(0, "not a number".to_f);
        assert_equal(Infinity, "Infinity".to_f);
        assert_equal(-Infinity, "-Infinity".to_f);
        assert("NaN".to_f.nan);
    }
    
    def test_inspect {
        assert_equal("\"hello\"", "hello".inspect);
        assert_equal("\"hel\\\"lo\"", "hel\"lo".inspect);
        assert_equal("\"\\\\\"", "\\".inspect);
    }
    
    def test_html_escape {
        assert_equal("&lt;script&gt;", "<script>".html_escape);
        assert_equal("&amp;", "&".html_escape);
        assert_equal("&amp;amp;", "&amp;".html_escape);
        assert_equal("&quot;", "\"".html_escape);
        assert_equal("&#039;", "'".html_escape);
    }
    
    def test_url_encode {
        assert_equal("hello+world", "hello world".url_encode);
        assert_equal("%2520", "%20".url_encode);
        assert_equal("%C2%AB%CF%84%CE%B1%D0%91%D0%AC%E2%84%93%CF%83%C2%BB", "«ταБЬℓσ»".url_encode);
    }
    
    def test_url_decode {
        assert_equal("hello world", "hello+world".url_decode);
        assert_equal("hello world", "hello%20world".url_decode);
        assert_equal("%20", "%2520".url_decode);
        assert_equal("«ταБЬℓσ»", "%C2%AB%CF%84%CE%B1%D0%91%D0%AC%E2%84%93%CF%83%C2%BB".url_decode);
    }
    
    def test_index {
        assert_equal(1, "hello".index("ello"));
        assert_equal(1, "hello-ello".index("ello"));
        assert_equal(nil, "hello".index("lol"));
        assert_equal(3, "«ταБЬℓσ»".index("Б"));
    }
    
    def test_char_at_index {
        assert_equal("a", "abcdef"[0]);
        assert_equal("b", "abcdef"[1]);
        assert_equal("f", "abcdef"[-1]);
        assert_equal("e", "abcdef"[-2]);
        
        assert_equal(nil, "abcdef"[100]);
        assert_equal(nil, "abcdef"[-100]);
        
        assert_equal("А", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[0]);
        assert_equal("Б", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[1]);
        assert_equal("В", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[2]);
        assert_equal("Г", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[3]);
        
        assert_equal("Я", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[-1]);
        assert_equal("Ю", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[-2]);
        assert_equal("Э", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[-3]);
        assert_equal("Ь", "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[-4]);
        
        assert_equal(nil, "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[100]);
        assert_equal(nil, "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"[-100]);
    }

    def test_range_index {
        assert_equal("h", "hello"[0..0]);
        assert_equal("", "hello"[0...0]);

        assert_equal("ell", "hello"[1..-2]);
        assert_equal("el", "hello"[1...-2]);

        assert_equal("ll", "hello"[-3..-2]);
        assert_equal("l", "hello"[-3...-2]);
    }
    
    def test_split {
        assert_equal(["a", "b", "c"], "abc".split(""));
        assert_equal(["hello"], "hello".split("x"));
        assert_equal(["foo", ""], "foobar".split("bar"));
        assert_equal(["", "bar"], "foobar".split("foo"));
        assert_equal(["", ""], "foobar".split("foobar"));
    }

    def test_split_with_limit {
        assert_equal(["abc"], "abc".split("", 1));
        assert_equal(["a", "bc"], "abc".split("", 2));
        assert_equal(["a", "b", "c"], "abc".split("", 3));

        assert_equal(["foo bar baz"], "foo bar baz".split(" ", 1));
        assert_equal(["foo", "bar baz"], "foo bar baz".split(" ", 2));
        assert_equal(["foo", "bar", "baz"], "foo bar baz".split(" ", 3));
    }

    def test_split_with_default_argument {
        assert_equal(["hello", "world"], "hello world".split);
        assert_equal(["a", "b", "c"], "a b c".split);
    }
    
    def test_spaceship {
        assert_equal(0, "a" <=> "a");
        assert_equal(-1, "a" <=> "b");
        assert_equal(-1, "" <=> "a");
        assert_equal(1, "b" <=> "a");
        assert_equal(1, "b" <=> "");
    }
    
    def test_replace_string_string {
        assert_equal("A::B::C", "A,B,C".replace(",", "::"));
    }
    
    def test_replace_regexp_string {
        assert_equal("A::B::C", "A1B2C".replace(%r{\d}, "::"));
    }
    
    def test_replace_regexp_lambda {
        assert_equal("A:B::C", "A<1>B<2>C".replace(%r{<(\d+)>}, \x { ":" * x[1].to_i }));
    }
    
    def test_lower {
        assert_equal("abcdef", "AbCdEf".lower);
        assert_equal("åéîøü", "ÅÉÎØÜ".lower);
    }
    
    def test_upper {
        assert_equal("ABCDEF", "AbCdEf".upper);
        assert_equal("ÅÉÎØÜ", "åéîøü".upper);
    }

    def test_tilde_operator {
        assert_equal(true, "hello world" ~ %r{hello});
        assert_equal(false, "goodbye world" ~ %r{hello});

        assert_equal(true, "hello world" ~ "hello");
        assert_equal(false, "goodbye world" ~ "hello");
    }
}
