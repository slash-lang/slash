<%

class TestError extends Test {
    def test_name {
        assert_equal("Error", Error.new.name);
        assert_equal("ArgumentError", ArgumentError.new.name);
        assert_equal("TestError::FooBarError", class FooBarError extends Error {}.new.name);
    }
    
    def test_message {
        assert_equal(nil, Error.new.message);
        assert_equal("foobar", Error.new("foobar").message);
    }
    
    def test_backtrace {
        try {
            \{ \{ throw Error.new; }.call; }.call;
        } catch e {
            assert_equal(3, e.backtrace.length);
            assert_equal("<lambda>", e.backtrace[0].method);
            assert_equal("<lambda>", e.backtrace[1].method);
            assert_equal("test_backtrace", e.backtrace[2].method);
        }
    }
    
    def test_stack_overflow_error {
        f = \{ f.call; };
        assert_throws(StackOverflowError, f);
    }
    
    def test_throwing_non_error_throws_type_error {
        assert_throws(TypeError, \{ throw 1; });
    }
}.register;