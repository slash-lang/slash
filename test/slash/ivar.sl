<%

class IVarTest extends Test {
    class A {
        def get_x_a {
            @x;
        }
        
        def set_x_a(x) {
            @x = x;
        }
    }
    
    class B extends A {
        def get_x_b {
            @x;
        }
        
        def set_x_b(x) {
            @x = x;
        }
    }
    
    def test_simple {
        a = A.new;
        assert_equal(nil, a.get_x_a);
        a.set_x_a(123);
        assert_equal(123, a.get_x_a);
    }
    
    def test_inherited {
        b = B.new;
        assert_equal(nil, b.get_x_a);
        b.set_x_a(123);
        assert_equal(123, b.get_x_b);
        b.set_x_b(456);
        assert_equal(456, b.get_x_a);
    }
    
    def test_ivars_are_not_shared {
        a = A.new;
        b = A.new;
        a.set_x_a(123);
        b.set_x_a(456);
        assert_equal(123, a.get_x_a);
        assert_equal(456, b.get_x_a);
    }
}