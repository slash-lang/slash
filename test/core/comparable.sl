<%

class TestComparable extends Test {
    class Always extends Comparable {
        def init(retn) {
            @retn = retn;
        }
        
        def <=>(other) {
            @retn;
        }
        
        Greater = new(1);
        Lesser = new(-1);
        Equal = new(0);
    }
    
    def test_lt {
        assert_equal(true, Always::Lesser < 0);
        assert_equal(false, Always::Greater < 0);
        assert_equal(false, Always::Equal < 0);
    }
    
    def test_lte {
        assert_equal(true, Always::Lesser <= 0);
        assert_equal(false, Always::Greater <= 0);
        assert_equal(true, Always::Equal <= 0);
    }
    
    def test_lt {
        assert_equal(false, Always::Lesser > 0);
        assert_equal(true, Always::Greater > 0);
        assert_equal(false, Always::Equal > 0);
    }
    
    def test_lte {
        assert_equal(false, Always::Lesser >= 0);
        assert_equal(true, Always::Greater >= 0);
        assert_equal(true, Always::Equal >= 0);
    }
}