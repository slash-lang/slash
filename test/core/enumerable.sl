<%

class EnumerableTest extends Test {
    class UselessEnumerable extends Enumerable {
        def enumerate {
            obj = Object.new;
            def obj.next {
                false;
            }
            obj;
        }
    }

    class LengthOfExactlyFourEnumerable extends Enumerable {
        def init {
            @times = 0;
        }

        def enumerate {
            self;
        }

        def next {
            if @times++ < 4 {
                true;
            } else {
                false;
            }
        }

        def current {
            @times;
        }
    }

    class InfiniteEnumerable extends Enumerable {
        def enumerate {
            obj = Object.new;
            def obj.next {
                true;
            }
            def obj.current {
                true;
            }
            obj;
        }
    }

    def test_map {
        assert_equal([], [].map(\x { x }));
        assert_equal([1,2,3], [1,2,3].map(\x { x }));
        assert_equal([1,4,9], [1,2,3].map(\x { x * x }));
    }

    def test_to_a {
        assert_equal([], [].to_a);
        assert_equal([], {}.to_a);
        assert_equal([["a", 1]], { "a" => 1 }.to_a);
    }

    def test_reduce {
        assert_throws(ArgumentError, \ {
            [].reduce(\(a,b) {});
        });
        assert_equal(0, [].reduce(0, \(a,b) {}));
        assert_equal(6, [1,2,3].reduce(\(a,b) { a + b }));
        assert_equal(10, [1,2,3].reduce(4, \(a,b) { a + b }));
        assert_equal(10, [1,2,3].fold(4, \(a,b) { a + b }));
    }

    def test_join {
        assert_equal("123", [1,2,3].join);
        assert_equal("", [].join);
        assert_equal("[1, 2]", { 1 => 2 }.join);

        assert_equal("1::2::3", [1,2,3].join("::"));
        assert_equal("", [].join("::"));
        assert_equal("[1, 2]", { 1 => 2 }.join("::"));
    }

    def test_length {
        assert_equal(4, { 1 => 2, 3 => 4, 5 => 6, 7 => 8 }.length);
        assert_equal(0, UselessEnumerable.new.length);
        assert_equal(4, LengthOfExactlyFourEnumerable.new.length);
    }

    def test_empty {
        assert_equal(true, [].empty);
        assert_equal(false, [1, 2, 3].empty);

        assert_equal(true, {}.empty);
        assert_equal(false, { 1 => 2 }.empty);

        assert_equal(false, InfiniteEnumerable.new.empty);
    }

    def test_any {
        assert_equal(false, [].any);
        assert_equal(true, [1, 2, 3].any);

        assert_equal(false, [false, false, false].any);

        assert_equal(false, [1,2,3].any(\x { x > 3 }));
        assert_equal(true, [1,2,3,4].any(\x { x > 3 }));

        assert_equal(true, InfiniteEnumerable.new.any);
    }

    def test_all {
        assert_equal(true, [].all);
        assert_equal(false, [false].all);
        assert_equal(true, [123].all);
        assert_equal(false, [4,5,6].all(\x { x >= 5 }));
        assert_equal(true, [4,5,6].all(\x { x >= 4 }));
    }

    def test_find {
        assert_equal(3, [1, 2, 3, 4, 5].find(\x { x > 2 }));
        assert_equal(nil, [].find(\x { x > 2 }));
        assert_equal(nil, [1, 2, -3, -4, -5].find(\x { x > 2 }));
    }

    def test_filter {
        assert_equal([3, 4, 5], [1, 2, 3, 4, 5].filter(\x { x > 2 }));
        assert_equal([], [].filter(\x { x > 2 }));
        assert_equal([], [1, 2, -3, -4, -5].filter(\x { x > 2 }));
    }

    def test_reject {
        assert_equal([1, 2], [1, 2, 3, 4, 5].reject(\x { x > 2 }));
        assert_equal([], [].reject(\x { x > 2 }));
        assert_equal([1, 2, -3, -4, -5], [1, 2, -3, -4, -5].reject(\x { x > 2 }));
    }

    def test_sort {
        assert_equal([1,2,3,4], Enumerable.instance_method("sort").bind([4,3,2,1]).call);
        assert_equal([], Enumerable.instance_method("sort").bind([]).call);

        assert_equal(["z", "cc", "aaa"], Enumerable.instance_method("sort").bind(["aaa", "cc", "z"]).call(\(a,b) { a.length <=> b.length }));
    }

    def test_take {
        assert_equal([], [1,2,3,4,5].take(0));
        assert_equal([1,2,3], [1,2,3,4,5].take(3));
        assert_equal([1,2,3,4,5], [1,2,3,4,5].take(9));
        assert_equal([1,2], [1,2].take(3));
    }

    def test_drop {
        assert_equal([1,2,3,4,5], [1,2,3,4,5].drop(0));
        assert_equal([4,5], [1,2,3,4,5].drop(3));
        assert_equal([], [1,2,3,4,5].drop(9));
        assert_equal([], [1,2].drop(3));
        assert_equal([], [1,2].drop(2));
    }

    def test_first {
        assert_equal(1, LengthOfExactlyFourEnumerable.new.first);
        assert_equal(nil, UselessEnumerable.new.first);
    }

    def test_first_works_on_infinite_enumerables {
        assert_equal(true, InfiniteEnumerable.new.first);
    }

    def test_last {
        assert_equal(4, LengthOfExactlyFourEnumerable.new.last);
        assert_equal(nil, UselessEnumerable.new.last);
    }

    def test_includes {
        assert(LengthOfExactlyFourEnumerable.new.includes(3));
        refute(LengthOfExactlyFourEnumerable.new.includes(5));
        assert(InfiniteEnumerable.new.includes(true));
    }

    def test_each {
        UselessEnumerable.new.each(\_ { flunk() });
        ary = [];
        LengthOfExactlyFourEnumerable.new.each(\x { ary.push(x) });
        assert_equal([1,2,3,4], ary);
    }

    def test_sum {
        assert_equal(0, [].sum);
        assert_equal(1, [1].sum);
        assert_equal(3, [1,2].sum);
    }
}
