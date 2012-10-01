<%

class ArrayTest extends Test {
    def test_init {
        assert_equal([], Array.new);
        assert_equal([1,2,3,4], Array.new(1, 2, 3, 4));
    }
    
    def test_enumerate {
        a = [1,2,3];
        enumerator = a.enumerate;
        assert_equal(true, enumerator.next);
        assert_equal(1, enumerator.current);
        assert_equal(true, enumerator.next);
        assert_equal(2, enumerator.current);
        assert_equal(true, enumerator.next);
        assert_equal(3, enumerator.current);
        assert_equal(false, enumerator.next);
    }
    
    def test_get {
        a = [1,2,3];
        assert_equal(1, a[0]);
        assert_equal(2, a[1]);
        assert_equal(3, a[2]);
        assert_equal(nil, a[1000]);
        assert_throws(TypeError, \{
            a["hello"]
        });
    }
    
    def test_set {
        a = [];
        a[3] = 123;
        assert_equal([nil, nil, nil, 123], a);
        a[0] = 456;
        assert_equal([456, nil, nil, 123], a);
        a[3] = nil;
        assert_equal([456, nil, nil, nil], a);
        assert_throws(TypeError, \{
            a["hello"] = 789;
        });
    }
    
    def test_length {
        assert_equal(0, [].length);
        assert_equal(4, [1,2,3,4].length);
    }
    
    def test_push {
        a = [];
        assert_equal([], a);
        assert_equal(1, a.push(1));
        assert_equal([1], a);
        assert_equal(3, a.push(1, 2));
        assert_equal([1, 1, 2], a);
        assert_equal(3, a.push);
        assert_equal([1, 1, 2], a);
    }
    
    def test_pop {
        a = [1, 2, 3];
        assert_equal([1, 2, 3], a);
        assert_equal(3, a.pop);
        assert_equal([1, 2], a);
        assert_equal(2, a.pop);
        assert_equal(1, a.pop);
        assert_equal([], a);
        assert_equal(nil, a.pop);
    }
    
    def test_unshift {
        a = [];
        assert_equal(1, a.unshift(1));
        assert_equal([1], a);
        assert_equal(3, a.unshift(2, 3));
        assert_equal([2, 3, 1], a);
        assert_equal(3, a.unshift);
        assert_equal([2, 3, 1], a);
    }
    
    def test_shift {
        a = [1, 2, 3];
        assert_equal([1, 2, 3], a);
        assert_equal(1, a.shift);
        assert_equal([2, 3], a);
        assert_equal(2, a.shift);
        assert_equal(3, a.shift);
        assert_equal([], a);
        assert_equal(nil, a.shift);
    }
    
    def test_to_a {
        assert_equal([], [].to_a);
        assert_equal([1, 2], [1, 2].to_a);
    }
    
    def test_to_s_and_inspect {
        assert_equal("[]", [].to_s);
        assert_equal("[1, 2]", [1, 2].to_s);
        assert_equal("[]", [].inspect);
        assert_equal("[1, 2]", [1, 2].inspect);
    }
    
    def test_inspect_with_recursive_array {
        a = [];
        a.push(a);
        assert_equal("[[ <recursive> ]]", a.to_s);
    }
    
    def test_sort {
        assert_equal([1,2,3,4], [4,3,2,1].sort);
    }
    
    def test_concat {
        assert_equal([], [] + []);
        assert_equal([1, 2, 3], [1, 2, 3] + []);
        assert_equal([1, 2, 3], [] + [1, 2, 3]);
        assert_equal([1, 2, 3, 4], [1, 2] + [3, 4]);
    }
}.register;