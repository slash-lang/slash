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
}.register;