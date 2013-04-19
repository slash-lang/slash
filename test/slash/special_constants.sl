<%

class SpecialConstantsTest extends Test {
    def test_line {
        assert_equal(5, __LINE__);
    }

    def test_file {
        assert(%r{test/slash/special_constants\.sl}.match(__FILE__));
    }
}
