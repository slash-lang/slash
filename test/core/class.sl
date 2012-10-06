<%

class ClassTest extends Test {
    class Foo {
        SOME_CONST = 456;
        
        MYMETHOD = def foo {
            123;
        };
    }
    
    class Bar extends Foo {
    }
        
    def test_to_s {
        assert_equal("ClassTest::Foo", Foo.to_s);
        assert_equal("ClassTest::Foo", Foo.inspect);
    }
    
    def test_name {
        assert_equal("Foo", Foo.name);
    }
    
    def test_in {
        assert_equal(ClassTest, Foo.in);
    }
    
    def test_super {
        assert_equal(Foo, Bar.super);
        assert_equal(Object, Foo.super);
        assert_equal(nil, Object.super);
    }
    
    def test_instance_method {
        assert_equal(Foo::MYMETHOD, Foo.instance_method("foo"));
        assert_equal(Foo::MYMETHOD, Bar.instance_method("foo"));
        assert_equal(nil, Foo.instance_method("hello"));
    }
    
    def test_own_instance_method {
        assert_equal(Foo::MYMETHOD, Foo.own_instance_method("foo"));
        assert_equal(nil, Bar.own_instance_method("foo"));
    }
    
    def test_instance_methods {
        assert_equal(Foo.instance_methods, Bar.instance_methods);
    }
    
    def test_own_instance_methods {
        assert_equal(["foo"], Foo.own_instance_methods);
        assert_equal([], Bar.own_instance_methods);
    }
    
    def test_get_constant {
        assert_equal(456, Foo::SOME_CONST);
        assert_equal(456, Bar::SOME_CONST);
        assert_throws(NameError, \{ Foo::NOT_SOME_CONST });
    }
    
    def test_get_constant_of_instance {
        assert_equal(456, Foo.new::SOME_CONST);
    }
    
    def test_set_constant {
        assert_throws(NameError, \{ Foo::IN_TEST_SET_CONSTANT });
        Foo::IN_TEST_SET_CONSTANT = "hello";
        assert_equal("hello", Foo::IN_TEST_SET_CONSTANT);
        assert_throws(NameError, \{ Foo::IN_TEST_SET_CONSTANT = "redefined"; });
    }
    
    def test_class {
        assert_equal(Class, Foo.class);
    }
}.register;