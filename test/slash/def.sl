<%

class DefTest extends Test {
    class TestDefOnInstance {
        def do_it {
            def method {
                123;
            }
        }
    }
    
    def test_def_on_instance {
        a = TestDefOnInstance.new;
        a.do_it;
        assert_equal(123, a.method);
        b = TestDefOnInstance.new;
        assert_equal(123, b.method);
    }
    
    def test_define_singleton_method {
        a = Object.new;
        b = Object.new;
        def a.foo { 123 }
        assert_equal(123, a.foo);
        assert_throws(NoMethodError, \{ b.foo });
    }
    
    def test_easter_egg {
        try {
            eval("def f(self) { }");
        } catch e {}
        assert_is_a(SyntaxError, e);
        assert_equal("not a chance", e.message);
    }
}