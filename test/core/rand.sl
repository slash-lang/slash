<%

class RandTest extends Test {
    def test_rand_returns_number_between_0_and_1 {
        assert(rand() > 0, "rand() > 0");
        assert(rand() < 1, "rand() < 0");
    }
}