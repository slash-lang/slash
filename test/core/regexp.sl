<%

class RegexpTest extends Test {
    def test_match_data {
        re = %r{hello (.*)!};
        md = re.match("hello world!");
        assert_equal("hello world!", md[0]);
        assert_equal("world", md[1]);
        assert_equal(2, md.length);
        assert_equal(re, md.regexp);
        
        md = %r{hello (.*)!}.match("goodbye world!");
        assert_equal(nil, md);
        
        md = %r{hello (?<what>.*)!}.match("hello world!");
        assert_equal("world", md["what"]);
        assert_equal(nil, md["not"]);
    }
    
    def test_init {
        re = Regexp.new("hello");
        assert(re.match("hello"), "/hello/ matches 'hello'");
        assert(!re.match("HELLO"), "/hello/ does not match 'HELLO'");
        
        re = Regexp.new("hello", "i");
        assert(re.match("hello"), "/hello/i matches 'hello'");
        assert(re.match("HELLO"), "/hello/i matches 'HELLO'");
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
        assert(%r{} == %r{}, "expected two empty regexps to be equal");
        assert(%r{foo} == %r{foo}, "expected two non-empty regexps to be equal");
        assert(%r{}i == %r{}i, "expected two empty regexps with options to be equal");
        assert(%r{foo}i == %r{foo}i, "expected two non-empty regexps with options to be equal");
        
        assert(%r{foo} != %r{bar}, "expected two different regexps to not be equal");
        assert(%r{foo}i != %r{foo}x, "expected two regexps with different options to not be equal");
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