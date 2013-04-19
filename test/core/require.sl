<%

class RequireTest extends Test {
    def test_throws_if_unable_to_require {
        assert_throws(Error, \{ require("i_do_not_exist"); });
    }
}