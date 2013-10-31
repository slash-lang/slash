<%

use Struct;

class DestructuringAssignmentTest extends Test {
    def test_array_assignment {
        [a, b, c] = [1, 2, 3];
        assert_equal(1, a);
        assert_equal(2, b);
        assert_equal(3, c);
    }

    def test_array_assignment_2 {
        [a, b, c] = [1, 2];
        assert_equal(1, a);
        assert_equal(2, b);
        assert_equal(nil, c);
    }

    def test_array_assignment_2 {
        [a, b, c] = 1;
        assert_equal(1, a);
        assert_equal(nil, b);
        assert_equal(nil, c);
    }

    def test_nested_array_assignment {
        [[a, b]] = [[1, 2], [3, 4]];

        assert_equal(1, a);
        assert_equal(2, b);
    }

    def test_nested_array_assignment_2 {
        [[a, b]] = [1, [2, 3]];

        assert_equal(1, a);
        assert_equal(nil, b);
    }

    def test_array_send_assignment {
        obj = Struct.new(['a, 'b]).new({});

        [[obj.a, b], obj.b] = [[1, 2], [3, 4]];

        assert_equal(1, obj.a);
        assert_equal(2, b);
        assert_equal([3, 4], obj.b);
    }
}
