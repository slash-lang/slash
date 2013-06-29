<%

class OperatorTest extends Test {
    def test_binary_tilde {
        [a, b] = [Object.new, Object.new];
        called = false;
        def a.~(other) {
            called = true;
        }
        a ~ b;
        assert(called);
    }
}
