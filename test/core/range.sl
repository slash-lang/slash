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
}.register;