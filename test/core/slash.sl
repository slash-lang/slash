<%

class SlashTest extends Test {
    def test_version {
        assert_is_a(String, Slash::VERSION);
    }
}
