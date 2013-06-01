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
            assert_equal("<lambda>", e.backtrace[0].method);
            assert_equal("call", e.backtrace[1].method);
            assert_equal("<lambda>", e.backtrace[2].method);
            assert_equal("call", e.backtrace[3].method);
            assert_equal("test_backtrace", e.backtrace[4].method);
        }
    }

    def test_backtrace_function {
        assert_is_an(Array, backtrace());
        assert_equal("test_backtrace_function", backtrace().first.method);
    }

    def test_stack_overflow_error {
        # FIXME: this test takes 200ms
        f = \{ f.call; };
        assert_throws(StackOverflowError, f);
    }

    def test_throwing_non_error_throws_type_error {
        assert_throws(TypeError, \{ throw 1; });
    }

    def test_yada_yada_operator {
        assert_throws(NotImplementedError, \{ ... });
    }

    def test_jump_out_of_try_block {
        assert_throws(TypeError, \{
            while true {
                try {
                    last;
                } catch e {
                    assert(false, "exception delivered to incorrect catch");
                }
            }
            throw TypeError.new;
        });
    }

    def test_throw {
        class X extends Error {}
        assert_throws(X, X.new:throw);
    }
}
