<%

class MethodTest extends Test {
    def test_class {
        assert_is_a(BoundMethod, 1.method("+"));
    }
    
    def test_call {
        assert_equal(3, 1.method("+").call(2));
        assert_equal(5, 2:+.call(3));
    }
    
    def test_apply {
        assert_equal(3, Int.instance_method("+").apply(1, 2));
        assert_equal(5, 1.method("+").apply(3, 2));
    }
    
    def test_unbind {
        assert_is_a(Method, 1.method("+").unbind);
    }
    
    def test_bind {
        assert_equal(3, 7.method("+").unbind.bind(1).call(2));
        assert_equal(3, 7.method("+").bind(1).call(2));
        assert_equal(3, Int.instance_method("+").bind(1).call(2));
        assert_is_a(BoundMethod, 7.method("+").unbind.bind(1));
        assert_is_a(BoundMethod, 7.method("+").bind(1));
        assert_is_a(BoundMethod, 7:+);
        assert_is_a(BoundMethod, 7:+.unbind.bind(1));
    }
    
    def test_bind_will_not_bind_to_unrelated_object {
        unbound = 1.method("+").unbind;
        assert_throws(TypeError, \{ unbound.bind("hello"); });
    }
    
    def test_cannot_call_uninitialized_method {
        bound_method = BoundMethod.new;
        assert_throws(TypeError, \{ bound_method.call });
    }
    
    def test_cannot_apply_uninitialized_method {
        method = Method.new;
        assert_throws(TypeError, \{ method.apply(nil); });
        bound_method = BoundMethod.new;
        assert_throws(TypeError, \{ bound_method.apply(nil) });
    }
    
    def test_cannot_bind_uninitialized_method {
        assert_throws(TypeError, \{ Method.new.bind(nil); });
    }
}.register;