<%

class SlashClassTest extends Test {
    class A {}

    class A::B {}

    def test_b_is_defined_inside_a {
        assert_equal(A, A::B.in);
    }
}
