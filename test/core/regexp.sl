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
}.register;