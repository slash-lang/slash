<%

class LambdaTest extends Test {
    def test_call {
        assert_equal(123, \{ 123 }.call);
        assert_equal(self, \{ self }.call);
    }

    def test_call_with_self {
        assert_equal("foo", \{ self }.call_with_self("foo"));
    }
    
    def test_unicode_syntax {
        f = λ x . x * x;
        assert_equal(81, f.call(9));
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
