<%

class TestObject extends Test {
    class Foo {
        def foo {}
        def bar {}
    }

    def test_own_method {
        assert_equal(method("foo"), Foo.new.own_method("test_own_method"));
        assert_equal(nil, Foo.new.own_method("exit"));
    }

    def test_own_methods {
        assert_equal(["foo", "bar"], Foo.new.own_methods());
    }

    def test_hash {
        assert_unequal(Object.new.hash, Object.new.hash);
    }

    def test_not_equal {
        o = Object.new;
        assert(o == o);
        refute(o != o);
        def o.==(x) { false }
        assert(o != o);

        called = 0;
        def o.==(x) { called++; true }
        refute(o != o);
        assert_equal(1, called);
    }

    def test_responds_to {
        assert(Foo.new.responds_to("foo"));
        assert(Foo.new.responds_to("exit"));
        refute(Foo.new.responds_to("baz"));

        o = Object.new;
        refute(o.responds_to("zzz"));
        def o.zzz {};
        assert(o.responds_to("zzz"));
    }

    def test_class {
        assert_equal(Object, Object.new.class);
        assert_equal(Class, Foo.class);
        assert_equal(Class, Class.class);
    }

    def test_method {
        assert_is_a(Method, method("test_method"));
        assert_equal(nil, method("what what what"));
    }
}.register;
