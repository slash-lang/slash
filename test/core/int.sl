<%

class IntTest extends Test {
    def test_succ {
        assert_equal(2, 1.succ);
        assert_equal(0, -1.succ);
    }
    
    def test_pred {
        assert_equal(1, 2.pred);
        assert_equal(-1, 0.pred);
    }
    
    def test_negate {
        assert_equal(-5, 5.negate);
        assert_equal(-(5), 5.negate);
    }
    
    def test_addition {
        assert_equal(123, 100 + 23);
        assert_equal(123.5, 100 + 23.5);
        assert_equal(1073741824, 536870912 + 536870912);
    }
    
    def test_subtraction {
        assert_equal(77, 100 - 23);
        assert_equal(99.5, 123 - 23.5);
        assert_equal(-1073741824, -536870912 - 536870912);
    }
    
    def test_multiplication {
        assert_equal(2300, 100 * 23);
        assert_equal(999999999999999999990, 10 * 99999999999999999999);
    }
    
    def test_division {
        assert_equal(4, 100 / 23);
        assert_throws(ZeroDivisionError, \{ 1 / 0 });
    }
    
    def test_modulo {
        assert_equal(8, 100 % 23);
        assert_equal(0.5, 10 % 9.5);
        assert_throws(ZeroDivisionError, \{ 1 % 0 });
    }
    
    def test_power {
        assert_equal(16, 2 ** 4);
        assert_equal(32, 2 ** 5);
        assert_equal(64, 2 ** 6);
        assert_equal(4, 16 ** 0.5);
        assert_equal(1, 2 ** Bignum.new);
        assert_equal(0.5, 2 ** -1);
    }
    
    def test_and {
        assert_equal(0, 127 & 0);
        assert_equal(14, 126 & 15);
        assert_equal(1, 1 & 18446744073709551615);
        assert_equal(0, 1 & 18446744073709551616);
    }
    
    def test_or {
        assert_equal(127, 127 | 0);
        assert_equal(127, 126 | 15);
        assert_equal(65535, 65534 | 3);
        assert_equal(18446744073709551615, 1 | 18446744073709551615);
        assert_equal(18446744073709551617, 1 | 18446744073709551616);
    }
    
    def test_xor {
        assert_equal(127, 127 ^ 0);
        assert_equal(113, 126 ^ 15);
        assert_equal(65532, 65535 ^ 3);
        assert_equal(18446744073709551614, 1 ^ 18446744073709551615);
        assert_equal(18446744073709551617, 1 ^ 18446744073709551616);
    }
    
    def test_not {
        assert_equal(-457, ~456);
        assert_equal(0, ~-1);
    }
    
    def test_to_s_and_inspect {
        assert_equal("5", 5.to_s);
        assert_equal("5", 5.inspect);
    }
    
    def test_to_i {
        assert_equal(100, 100.to_i);
        assert_is_a(Int, 100.to_i);
    }

    def test_round {
        assert_equal(100, 100.round);
    }
    
    def test_to_f {
        assert_equal(100.0, 100.to_f);
        assert_is_a(Float, 100.to_f);
    }
    
    def test_inequality {
        assert_unequal(100, 200);
        assert_unequal(100, nil);
    }
    
    def test_spaceship {
        assert_equal(0, 100 <=> 100);
        assert_equal(-1, 100 <=> 200);
        assert_equal(1, 200 <=> 100);
        assert_equal(0, 100 <=> 100.0);
        assert_equal(-1, 100 <=> 100.5);
        assert_equal(1, 100 <=> 99.5);
        assert_equal(0, 0 <=> Bignum.new);
        assert_equal(-1, 100 <=> 999999999999999999999999);
        assert_equal(1, 100 <=> -999999999999999999999999);
    }
    
    def test_class {
        assert_is_a(Int, 123);
    }
}.register;
