<%

class RegexpTest extends Test {
    class UninitializedRegexp extends Regexp {
        def init {}
    }
    
    def test_match_data {
        re = %r{hello (.*)!};
        md = re.match("hello world!");
        assert_equal("hello world!", md[0]);
        assert_equal("world", md[1]);
        assert_equal(nil, md[2]);
        assert_equal(2, md.length);
        assert_equal(re, md.regexp);
        
        md = %r{hello (.*)!}.match("goodbye world!");
        assert_equal(nil, md);
        
        md = %r{hello (?<what>.*)!}.match("hello world!");
        assert_equal("world", md["what"]);
        assert_equal(nil, md["not"]);
    }
    
    def test_match_data_offset {
        re = %r{héllö (.*)!};
        md = re.match("héllö world!");
        assert_equal(0, md.offset(0));
        assert_equal(6, md.offset(1));
        assert_equal(nil, md.offset(2));
    }
    
    def test_match_data_byte_offset {
        re = %r{héllö (.*)!};
        md = re.match("héllö world!");
        assert_equal(0, md.byte_offset(0));
        assert_equal(8, md.byte_offset(1));
        assert_equal(nil, md.byte_offset(2));
    }
    
    def test_match_data_capture {
        re = %r{héllö (.*)!};
        md = re.match("héllö world!");
        assert_equal([0, 12], md.capture(0));
        assert_equal([6, 5], md.capture(1));
        assert_equal(nil, md.capture(2));
    }
    
    def test_match_data_after {
        re = %r{world};
        md = re.match("héllö world!ß");
        assert_equal("héllö ", md.before);
        assert_equal("!ß", md.after);
        md = re.match("world");
        assert_equal("", md.before);
        assert_equal("", md.after);
    }
    
    def test_match_from_offset {
        re = %r{\d+};
        md = re.match("123 456 789", 3);
        assert_equal("456", md[0]);
    }
    
    def test_init {
        re = Regexp.new("hello");
        assert(re.match("hello"), "/hello/ matches 'hello'");
        assert(!re.match("HELLO"), "/hello/ does not match 'HELLO'");
        
        re = Regexp.new("hello", "i");
        assert(re.match("hello"), "/hello/i matches 'hello'");
        assert(re.match("HELLO"), "/hello/i matches 'HELLO'");
    }
    
    def test_uninitialized_regexp_throws {
        re = UninitializedRegexp.new;
        assert_throws(TypeError, \{ re.match("lol"); });
    }
    
    def test_uninitialized_regexp_match_throws {
        class UninitializedRegexpMatch extends Regexp::Match {
            def init {}
        }
        match = UninitializedRegexpMatch.new;
        assert_throws(TypeError, \{ match.regexp });
    }
    
    def test_throws_on_invalid_regexp_option {
        assert_throws(ArgumentError, \{ Regexp.new("", "lol"); });
    }
    
    def test_throws_on_regexp_with_null_byte {
        assert_throws(ArgumentError, \{ Regexp.new("foo\x00bar"); });
    }
    
    def test_throws_syntax_error_on_bad_regexp {
        assert_throws(SyntaxError, \{ Regexp.new("["); });
    }
    
    def test_options {
        re = %r{hello};
        assert(re.match("hello"), "/hello/ matches 'hello'");
        assert(!re.match("HELLO"), "/hello/ does not match 'HELLO'");
        
        re = %r{hello}i;
        assert(re.match("hello"), "/hello/i matches 'hello'");
        assert(re.match("HELLO"), "/hello/i matches 'HELLO'");
    }
    
    def test_eq {
        assert_equal(%r{}, %r{});
        assert_equal(%r{foo}, %r{foo});
        assert_equal(%r{}i, %r{}i);
        assert_equal(%r{foo}i, %r{foo}i);
        
        assert_unequal(%r{foo}, %r{bar});
        assert_unequal(%r{foo}i, %r{foo}x);
        
        assert_unequal(%r{}, UninitializedRegexp.new);
        assert_unequal(%r{}, nil);
    }
    
    def test_source {
        assert_equal("", %r{}.source);
        assert_equal("foo", %r{foo}.source);
    }
    
    def test_options {
        assert_equal(Regexp::CASELESS, %r{}i.options);
        assert_equal(Regexp::EXTENDED, %r{}x.options);
        assert_equal(Regexp::CASELESS | Regexp::EXTENDED, %r{}ix.options);
        assert_equal(Regexp::CASELESS | Regexp::EXTENDED, %r{}xi.options);
    }

    def test_tilde_operator {
        assert_equal(true, %r{hello} ~ "hello world");
        assert_equal(false, %r{hello} ~ "goodbye world");
    }
}
