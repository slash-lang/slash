<%

class VarArgsTest extends Test {
    def var_arg(*x) {
        x;
    }

    def test_var_arg {
        assert_equal([1, 2, 3], var_arg(1, 2, 3));
        assert_equal([], var_arg());
    }

    def fixed_and_var_arg(a, b, *x) {
        [a, b, x];
    }

    def test_fixed_and_var_arg {
        assert_equal([1, 2, [3, 4, 5]], fixed_and_var_arg(1, 2, 3, 4, 5));
        assert_equal([1, 2, [3]], fixed_and_var_arg(1, 2, 3));
        assert_equal([1, 2, []], fixed_and_var_arg(1, 2));
        assert_throws(ArgumentError, \. fixed_and_var_arg(1));
    }

    def optional_and_var_arg(a = 'a, b = 'b, *x) {
        [a, b, x];
    }

    def test_optional_and_var_arg {
        assert_equal(['a, 'b, []], optional_and_var_arg());
        assert_equal([9, 'b, []], optional_and_var_arg(9));
        assert_equal([9, 8, []], optional_and_var_arg(9, 8));
        assert_equal([9, 8, [7]], optional_and_var_arg(9, 8, 7));
    }

    def fixed_optional_and_var_arg(a, b = 'b, *x) {
        [a, b, x];
    }

    def test_fixed_optional_and_var_arg {
        assert_throws(ArgumentError, \. fixed_optional_and_var_arg());
        assert_equal([9, 'b, []], fixed_optional_and_var_arg(9));
        assert_equal([9, 8, []], fixed_optional_and_var_arg(9, 8));
        assert_equal([9, 8, [7]], fixed_optional_and_var_arg(9, 8, 7));
    }
}
