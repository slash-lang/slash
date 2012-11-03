<%

class IncrementDecrementTest extends Test {
    class AccessCounter {
        def init {
            @count = 0;
        }
        
        def count {
            @count;
        }
        
        def val {
            @count++;
            @val;
        }
        
        def val=(val) {
            @val = val;
        }
    }
    
    def test_postfix_increment {
        x = 0;
        assert_equal(0, x++);
        assert_equal(1, x++);
        assert_equal(2, x);
    }
    
    def test_postfix_decrement {
        x = 0;
        assert_equal(0, x--);
        assert_equal(-1, x--);
        assert_equal(-2, x);
    }
    
    def test_prefix_increment {
        x = 0;
        assert_equal(1, ++x);
        assert_equal(2, ++x);
        assert_equal(2, x);
    }
    
    def test_prefix_decrement {
        x = 0;
        assert_equal(-1, --x);
        assert_equal(-2, --x);
        assert_equal(-2, x);
    }
    
    def test_evaluates_operand_once {
        a = AccessCounter.new;
        a.val = [0];
        assert_equal(0, a.count);
        
        assert_equal(0, a.val[0]++);
        assert_equal(2, ++a.val[0]);
        assert_equal(2, a.val[0]--);
        assert_equal(0, --a.val[0]);
        
        assert_equal(4, a.count);
    }
}.register;