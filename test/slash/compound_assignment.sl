<%

class CompoundAssignmentTest extends Test {
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
    
    def test_compound_assignment {
        x = 2;
        x += 3;
        assert_equal(5, x);
        
        @x = 4;
        @x += 5;
        assert_equal(9, @x);
        
        @@x = 6;
        @@x += 7;
        assert_equal(13, @@x);
        
        ary = [];
        ary[0] = 8;
        ary[0] += 9;
        assert_equal(17, ary[0]);
    }
    
    def test_compound_assignment_only_evaluates_lhs_once {
        counter = AccessCounter.new;
        counter.val = [];
        assert_equal(0, counter.count);
        
        counter.val[0] = 10;
        counter.val[0] += 11;
        assert_equal(2, counter.count);
        
        assert_equal(21, counter.val[0]);
    }
    
    def test_conditional_assignment {
        x = false;
        x ||= 3;
        assert_equal(3, x);
        
        x = 2;
        x ||= 3;
        assert_equal(2, x);
        
        @x = false;
        @x ||= 3;
        assert_equal(3, @x);
        
        @x = 2;
        @x ||= 3;
        assert_equal(2, @x);
        
        @@x = false;
        @@x ||= 3;
        assert_equal(3, @@x);
        
        @@x = 2;
        @@x ||= 3;
        assert_equal(2, @@x);
        
        ary = [];
        ary[0] ||= 3;
        assert_equal(3, ary[0]);
        
        ary[0] = 2;
        ary[0] ||= 3;
        assert_equal(2, ary[0]);
    }
    
    def test_conditional_assignment_only_evaluates_lhs_once {
        counter = AccessCounter.new;
        counter.val = [];
        assert_equal(0, counter.count);
        
        counter.val[0] ||= 10;
        assert_equal(1, counter.count);
        
        assert_equal(10, counter.val[0]);
    }
}