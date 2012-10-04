<%

class CVarTest extends Test {
    class A {
        def x=(x) {
            @@x = x;
        }
        
        def x {
            @@x;
        }
        
        def self.x=(x) {
            @@x = x;
        }
        
        def self.x {
            @@x;
        }
    }
    
    class B extends A {
        
    }
    
    def before {
        A.x = nil;
    }
    
    def test_cvar_on_class {
        assert_equal(nil, A.x);
        A.x = 123;
        assert_equal(123, A.x);
    }
    
    def test_cvar_on_instances {
        a = A.new;
        b = A.new;
        
        assert_equal(nil, a.x);
        assert_equal(nil, b.x);
        a.x = 123;
        assert_equal(123, a.x);
        assert_equal(123, b.x);
        b.x = 456;
        assert_equal(456, a.x);
        assert_equal(456, b.x);
    }
    
    def test_cvar_on_class_and_instances {
        a = A.new;
        b = A;
        
        assert_equal(nil, a.x);
        assert_equal(nil, b.x);
        a.x = 123;
        assert_equal(123, a.x);
        assert_equal(123, b.x);
        b.x = 456;
        assert_equal(456, a.x);
        assert_equal(456, b.x);
    }
}.register;