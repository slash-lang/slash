<%

class SuperTest extends Test {
    class X {
        def foo {
            "X"
        }
    }

    class Y extends X {
        def foo {
            ["Y", super];
        }
    }

    def test_super {
        assert_equal(["Y", "X"], Y.new.foo);
    }

    def method_calls_super {
        super;
    }

    def test_raises_no_method_error_when_no_superclass_method_exists {
        ex = assert_throws(NoMethodError, \. method_calls_super());
        assert_equal("Undefined method \"method_calls_super\" in Test for #{inspect()}", ex.message);
    }

    lambda_calls_super = \. super;

    def test_raises_no_method_error_when_called_in_non_method_context {
        ex = assert_throws(NoMethodError, \. lambda_calls_super.call);
        assert_equal("Can't call super outside of method context", ex.message);
    }
}
