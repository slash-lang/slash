<%

class LambdaTest extends Test {
    def test_call {
        assert_equal(123, \{ 123 }.call);
    }
    
    def test_throws_on_too_few_arguments {
        assert_throws(ArgumentError, \{
            \(a, b) { a + b }.call(1);
        });
    }
    
    def test_ignores_too_many_arguments {
        assert_equal(5, \(a, b) { a + b }.call(2, 3, 7));
    }
    
    def test_class {
        assert_is_a(Lambda, \{ });
        assert_is_a(Lambda, lambda { });
    }
}.register;