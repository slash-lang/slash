<%

class ArrayTest extends Test {
    def test_init {
        assert_equal([], Array.new);
        assert_equal([1,2,3,4], Array.new(1, 2, 3, 4));
    }
    
    def test_enumerate {
        a = [1,2,3];
        enumerator = a.enumerate;
        assert_throws(Error, \{ enumerator.current });
        assert_equal(true, enumerator.next);
        assert_equal(1, enumerator.current);
        assert_equal(true, enumerator.next);
        assert_equal(2, enumerator.current);
        assert_equal(true, enumerator.next);
        assert_equal(3, enumerator.current);
        assert_equal(false, enumerator.next);
        assert_equal(false, enumerator.next);
        assert_throws(Error, \{ enumerator.current });
    }
    
    def test_enumerator_throws_when_invalid {
        class AESub extends Array::Enumerator {
            def init {}
        }
        enumerator = AESub.new;
        assert_throws(Error, \{ enumerator.current });
        assert_throws(Error, \{ enumerator.next });
    }
    
    def test_get {
        a = [1,2,3];
        assert_equal(1, a[0]);
        assert_equal(2, a[1]);
        assert_equal(3, a[2]);
        assert_equal(3, a[-1]);
        assert_equal(2, a[-2]);
        assert_equal(1, a[-3]);
        assert_equal(nil, a[1000]);
        assert_equal(nil, a[-1000]);
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
        a[-2] = "foo";
        assert_equal([456, nil, "foo", nil], a);
        assert_throws(TypeError, \{
            a["hello"] = 789;
        });
        assert_throws(ArgumentError, \{
            a[-1000] = 789;
        });
    }
    
    def test_resizes_on_set {
        a = [];
        assert_equal(0, a.length);
        a[0] = 1;
        assert_equal(1, a.length);
        a[1] = 2;
        assert_equal(2, a.length);
        a[99] = 1;
        assert_equal(100, a.length);
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
        assert_equal(["z", "cc", "aaa"], ["aaa", "cc", "z"].sort(\(a,b) { a.length <=> b.length }));
    }
    
    def test_concat {
        assert_equal([], [] + []);
        assert_equal([1, 2, 3], [1, 2, 3] + []);
        assert_equal([1, 2, 3], [] + [1, 2, 3]);
        assert_equal([1, 2, 3, 4], [1, 2] + [3, 4]);
    }

    def test_diff {
        assert_equal([], [] - []);
        assert_equal([1, 2, 3], [1, 2, 3] - [4, 5, 6]);
        assert_equal([2], [1, 2, 3] - [1, 3]);
        assert_equal([2], [1, 1, 2, 3] - [1, 3]);
    }
    
    def test_resize {
        a = [];
        for i in 1..100 {
            a.push(i);
        }
        for i in 1..100 {
            a.pop();
            assert_equal(100 - i, a.length);
        }
    }
    
    def test_eq {
        assert_equal([1,2,3,4], [1,2,3,4]);
        assert_unequal([1,2,3,4], [1,2,3,4,5]);
        assert_unequal([1,2,3,4], [5,6,7,8]);
        assert_unequal([1,2,3,4], []);
        assert_unequal([], true);
    }
    
    def test_array_hash {
        assert_equal([1,2,3,4].hash, [1,2,3,4].hash);
        assert_unequal([5,6,7,8].hash, [1,2,3,4].hash);
    }
}