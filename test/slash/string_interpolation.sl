<%

class StringInterpolationTest extends Test {
    def test_entire_string_is_interpolated {
        assert_equal("16", "#{7 + 9}");
    }

    def test_with_left_padding {
        assert_equal("   16", "   #{7 + 9}");
    }

    def test_with_right_padding {
        assert_equal("16   ", "#{7 + 9}   ");
    }

    def test_with_both_padding {
        assert_equal("   16   ", "   #{7 + 9}   ");
    }

    def test_multiple_interpolations_with_spacing {
        assert_equal("1 + 2 = 3", "#{1} + #{2} = #{1 + 2}");
    }

    def test_multiple_interpolations_without_spacing {
        assert_equal("foobar", "#{'foo}#{'bar}");
    }
}
