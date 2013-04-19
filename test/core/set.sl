<%

require("set");

class SetTest extends Test {
    def test_items {
        assert_equal([1, 2, 3], Set.new([1, 2, 3]).items.sort);
    }
    
    def test_items_are_unique {
        assert_equal([1, 2, 3], Set.new([1, 2, 3, 1, 2, 3]).items.sort);
    }
    
    def test_add {
        set = Set.new;
        assert_equal(true, set.add("foo"));
        assert_equal(true, set.add("bar"));
        assert_equal(false, set.add("foo"));
        assert_equal(["bar", "foo"], set.items.sort);
    }
    
    def test_delete {
        set = Set.new([1, 2, 3]);
        assert_equal(false, set.delete(4));
        assert_equal(true, set.delete(2));
        assert_equal([1, 3], set.items.sort);
    }
    
    def test_has {
        set = Set.new([1, 2, 3]);
        assert_equal(false, set.has(4));
        assert_equal(true, set.has(1));
    }
    
    def test_length {
        assert_equal(0, Set.new.length);
        assert_equal(3, Set.new([1, 2, 3]).length);
        set = Set.new;
        set.add("foo");
        set.add("foo");
        set.add("bar");
        assert_equal(2, set.length);
    }
    
    def test_inspect {
        assert_equal("#<Set:{}>", Set.new.inspect);
        assert_equal("#<Set:{ 1 }>", Set.new([1]).inspect);
    }
    
    def test_enumerate {
        enumerator = Set.new.enumerate;
        assert_equal(false, enumerator.next);
        
        enumerator = Set.new([1]).enumerate;
        assert_equal(true, enumerator.next);
        assert_equal(1, enumerator.current);
        assert_equal(false, enumerator.next);
    }
    
    def test_equality {
        assert_equal(Set.new, Set.new);
        assert_equal(Set.new([1, 2]), Set.new([2, 1]));
        assert_equal(Set.new([1, 2, 3, 4]), Set.new([2, 1, 3, 3, 4, 3]));
    }
}