<%

class BracketedSeqTest extends Test {
    def test_bracketed_seq {
        assert_equal(3, (1; 2; 3));
        assert_equal(16, (b = 2 * 2; b * b))
    }
}
