<%

class EvalTest extends Test {
    def test_eval {
        assert_equal(3, eval("1 + 2"));
    }
    
    def test_eval_forwards_self {
        ary = [];
        ary.eval("push(1, 2, 3)");
        assert_equal([1, 2, 3], ary);
    }
    
    def test_eval_throws_on_syntax_error {
        assert_throws(SyntaxError, \{ eval("@#%@#$^@") });
    }
}.register;