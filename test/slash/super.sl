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
        assert_throws(NoMethodError, \. calls_super());
    }

    lambda_calls_super = \. super;

    def test_raises_no_method_error_when_calls_in_non_method_context {
        assert_throws(NoMethodError, \. lambda_calls_super.call);
    }
}
