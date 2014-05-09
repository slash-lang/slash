<%

class StringEscapesTest extends Test {
    def test_u {
        assert_equal("\x00", "\u0");
        assert_equal(" ", "\u20");
        assert_equal("†", "\u2020");

        assert_throws(SyntaxError, \{
            eval("\"\\u0000000\"");    
        });
    }
}
