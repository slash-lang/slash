<%

class RangeTest extends Test {
    def test_inclusive_range {
        assert_equal([1,2,3], (1..3).to_a);
    }
    
    def test_exclusive_range {
        assert_equal([1,2], (1...3).to_a);
    }
    
    def test_reverse_range_is_empty {
        assert_equal([], (1000..1).to_a);
    }
    
    def test_init {
        assert_equal([1,2,3,4], Range.new(1, 4).to_a);
        assert_equal([1,2,3], Range.new(1, 4, true).to_a);
    }

    def test_lower {
        assert_equal(1, (1..2).lower);
        assert_equal(true, (true..false).lower);
    }

    def test_upper {
        assert_equal(2, (1..2).upper);
        assert_equal(false, (true..false).upper);
    }
    
    def test_enumerate {
        enumerator = (0...0).enumerate;
        assert_throws(TypeError, \{ enumerator.current });
        assert_equal(false, enumerator.next);
        assert_throws(TypeError, \{ enumerator.current });
        assert_equal(false, enumerator.next);
    }
    
    class MyOrdinalObject extends Comparable {
        def init(dots) {
            @dots = dots;
        }
        
        def dots {
            @dots;
        }
        
        def succ {
            MyOrdinalObject.new(@dots + ".");
        }
        
        def <=>(other) {
            @dots.length <=> other.dots.length;
        }
    }
    
    def test_range_works_on_arbitrary_objects {
        a = MyOrdinalObject.new("..");
        b = MyOrdinalObject.new(".....");
        assert_equal(["..", "...", "....", "....."], (a..b).map(\x { x.dots }));
        assert_equal(["..", "...", "...."], (a...b).map(\x { x.dots }));
    }

    def test_uniterable_range {
        assert_throws(TypeError, \{ (nil..nil).to_a });
    }
}
