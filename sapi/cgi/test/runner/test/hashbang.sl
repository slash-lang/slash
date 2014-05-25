<%

class HashbangTest extends Test {
    def test_nohashbang {
        assert_equal(0, CgiTest.get_hashbang_length(""));
        assert_equal(0, CgiTest.get_hashbang_length("Hello World"));
        assert_equal(0, CgiTest.get_hashbang_length("#!Hello World"));
        assert_equal(0, CgiTest.get_hashbang_length("#!/Hello World"));
    }

    def test_newlines {
        assert_equal(21, CgiTest.get_hashbang_length("#!/usr/bin/slash-cgi\nTest\n"));
        assert_equal(21, CgiTest.get_hashbang_length("#!/usr/bin/slash-cgi\rTest\r"));
        assert_equal(22, CgiTest.get_hashbang_length("#!/usr/bin/slash-cgi\r\nTest\r\n"));
    }
}
