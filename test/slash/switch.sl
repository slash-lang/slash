<%

class SwitchTest extends Test {
    def test_switch_cases {
        res = switch 123 {
            123 {
                "123";
            }
            456 {
                flunk();
            }
            else {
                flunk();
            }
        };
        assert_equal("123", res);

        res = switch 456 {
            123 {
                flunk();
            }
            456 {
                "456";
            }
            else {
                flunk();
            }
        };
        assert_equal("456", res);
    }

    def test_switch_else {
        res = switch 789 {
            123 {
                flunk();
            }
            456 {
                flunk();
            }
            else {
                "else"
            }
        };
        assert_equal("else", res);
    }

    def test_switch_returns_nil_without_explicit_else {
        res = "not nil";
        res = switch 123 {};
        assert_equal(nil, res);
    }

    def test_switch_calls_eq {
        o = Object.new;
        called = false;
        def o.==(other) {
            called = other;
        }
        switch 123 {
            o {
                true;
            }
        }
        assert_equal(called, 123);
    }
}
