<%

class FalseTest extends Test {
    def test_to_s {
        assert_equal("false", false.to_s);
    }
    
    def test_inspect {
        assert_equal("false", false.inspect);
    }
    
    def test_equality {
        assert_equal(false, False.new);
        assert_unequal(false, true);
        assert(False.new == False.new, "Expected two different instances of False to be considered equal");
    }
    
    def test_is_truthy {
        flunk() if false;
        assert_equal(true, !false);
    }
    
    def test_class {
        assert_equal(False, false.class);
    }
}