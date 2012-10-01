<%

class TrueTest extends Test {
    def test_to_s {
        assert_equal("true", true.to_s);
    }
    
    def test_inspect {
        assert_equal("true", true.inspect);
    }
    
    def test_equality {
        assert_equal(true, True.new);
        assert(True.new == True.new, "Expected two different instances of True to be considered equal");
    }
    
    def test_is_truthy {
        flunk() unless true;
        assert_equal(false, !true);
    }
    
    def test_class {
        assert_equal(True, true.class);
    }
}.register;