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
    
    def test_eval_without_filename {
        frame = nil;
        try {
            eval("throw Error.new");
        } catch e {
            frame = e.backtrace[0];
        }
        assert_equal("(eval)", frame.file);
    }
    
    def test_eval_with_filename {
        frame = nil;
        try {
            eval("throw Error.new", "some file.txt");
        } catch e {
            frame = e.backtrace[0];
        }
        assert_equal("some file.txt", frame.file);
    }
}