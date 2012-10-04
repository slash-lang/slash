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
    
    def test_concat {
        assert_equal("", "" + "");
        assert_equal("foobar", "foo" + "bar");
        assert_equal("АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ", "АБВГДЕЖЗИЙКЛМНО" + "ПРСТУФХЦЧШЩЪЫЬЭЮЯ");
    }
    
    def test_to_s {
        assert_equal("", "".to_s);
        assert_equal("hello", "hello".to_s);
    }
    
    def test_to_i {
        assert_equal(123, "123".to_i);
        assert_equal(100000000000000000000000000000000000000, "100000000000000000000000000000000000000".to_i);
        assert_is_a(Bignum, "100000000000000000000000000000000000000".to_i);
        assert_equal(-999, "-999".to_i);
        assert_equal(2, "2.99".to_i);
        assert_equal(7, "     7".to_i);
        assert_equal(0, " not 7".to_i);
    }
}.register;