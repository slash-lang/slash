<%

class StringTest extends Test {
    def test_length {
        assert_equal(0, "".length);
        assert_equal(5, "hello".length);
        assert_equal(8, "«ταБЬℓσ»".length);
        assert_equal(11, "Justice - †".length);
        assert_equal(12, "いろはにほへとちりぬるを".length);
        assert_equal(52, "Γαζέες καὶ μυρτιὲς δὲν θὰ βρῶ πιὰ στὸ χρυσαφὶ ξέφωτο".length);
        assert_equal(32, "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ".length);
    }
}.register;