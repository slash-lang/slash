<%

class BignumTest extends Test {
    def test_the_numbers_we_are_using_are_big_enough {
        assert_is_a(Bignum, 1000000000000000000000000000000000000000);
    }
    
    def test_succ {
        assert_equal(1000000000000000000000000000000000000001, 1000000000000000000000000000000000000000.succ);
        assert_equal(-999999999999999999999999999999999999999, -1000000000000000000000000000000000000000.succ);
    }
    
    def test_pred {
        assert_equal(1000000000000000000000000000000000000000, 1000000000000000000000000000000000000001.pred);
        assert_equal(-1000000000000000000000000000000000000000, -999999999999999999999999999999999999999.pred);
    }
    
    def test_negate {
        assert_equal(-5000000000000000000000000000000000000000, 5000000000000000000000000000000000000000.negate);
        assert_equal(-(5000000000000000000000000000000000000000), 5000000000000000000000000000000000000000.negate);
    }
    
    def test_addition {
        assert_equal(2000000000000000000000000000000000000000, 1000000000000000000000000000000000000000 + 1000000000000000000000000000000000000000);
        assert_equal(1000000000000000000000000000000000000001, 1 + 1000000000000000000000000000000000000000);
        assert_equal(1000000000000000000000000000000000000001, 1000000000000000000000000000000000000000 + 1);
        assert_is_a(Float, 1000000000000000000000000000000000000000 + 1.0);
    }
    
    def test_subtraction {
        assert_equal(1000000000000000000000000000000000000000, 2000000000000000000000000000000000000000 - 1000000000000000000000000000000000000000);
        assert_equal(-999999999999999999999999999999999999999, 1 - 1000000000000000000000000000000000000000);
        assert_equal(999999999999999999999999999999999999999, 1000000000000000000000000000000000000000 - 1);
        assert_is_a(Float, 1000000000000000000000000000000000000000 - 1.0);
    }
    
    def test_multiplication {
        assert_equal(1000000000000000000000000000000000000000000000000000000000000000000000000000000, 1000000000000000000000000000000000000000 * 1000000000000000000000000000000000000000);
        assert_equal(1000000000000000000000000000000000000000, 100000000 * 100000000 * 100000000 * 100000000 * 10000000);
        assert_is_a(Float, 1000000000000000000000000000000000000000 * 2.0);
    }
    
    def test_division {
        assert_equal(1, 1000000000000000000000000000000000000000 / 1000000000000000000000000000000000000000);
        assert_equal(0, 0 / 1000000000000000000000000000000000000000);
        assert_equal(500000000000000000000000000000000000000, 1000000000000000000000000000000000000000 / 2);
        assert_equal(3, 1000000000000000000000000000000000000000 / 300000000000000000000000000000000000000);
        assert_is_a(Float, 1000000000000000000000000000000000000000 / 2.0);
        assert_throws(ZeroDivisionError, \{ 1000000000000000000000000000000000000000 / Bignum.new });
    }
    
    def test_modulo {
        assert_equal(1, 1000000000000000000000000000000000000000 % 3);
        assert_equal(100000000000000000000000000000000000000, 1000000000000000000000000000000000000000 % 300000000000000000000000000000000000000);
        assert_equal(72, 72 % 300000000000000000000000000000000000000);
        assert_is_a(Float, 1000000000000000000000000000000000000000 % 2.0);
        assert_throws(ZeroDivisionError, \{ 1000000000000000000000000000000000000000 % Bignum.new });
    }
    
    def test_power {
        assert_equal(1000000000000000000000000000000000000000000000000000000000000000000000000000000, 1000000000000000000000000000000000000000 ** 2);
        assert_equal(1, 1000000000000000000000000000000000000000 ** Bignum.new);
        assert_equal(10000, 100000000000000000000000000000000 ** 0.125);
        assert_equal(Infinity, Bignum.new ** -1);
        assert_equal(Infinity, Bignum.new ** -1.0);
        assert_throws(ArgumentError, \{ 1000000000000000000000000000000000000000 ** 1000000000000000000000000000000000000000 });
    }
    
    def test_to_s_and_inspect {
        assert_equal("1000000000000000000000000000000000000000", 1000000000000000000000000000000000000000.to_s);
        assert_equal("1000000000000000000000000000000000000000", 1000000000000000000000000000000000000000.inspect);
    }
    
    def test_to_i {
        assert_equal(1000000000000000000000000000000000000000, 1000000000000000000000000000000000000000.to_i);
        assert_is_a(Bignum, 1000000000000000000000000000000000000000.to_i);
    }

    def test_round {
        assert_equal(1000000000000000000000000000000000000000, 1000000000000000000000000000000000000000.round);
    }
    
    def test_to_f {
        assert_is_a(Float, 1000000000000000000000000000000000000000.to_f);
    }
    
    def test_eq {
        assert_equal(Bignum.new, 0);
        assert_equal(Bignum.new, 0.0);
    }
    
    def test_inequality {
        assert_unequal(1000000000000000000000000000000000000000, 1000000000000000000000000000000000000001);
        assert_unequal(1000000000000000000000000000000000000000, nil);
        assert_unequal(1000000000000000000000000000000000000000, 5.5);
    }
    
    def test_spaceship {
        assert_equal(0, 1000000000000000000000000000000000000000 <=> 1000000000000000000000000000000000000000);
        assert_equal(-1, 1000000000000000000000000000000000000000 <=> 1000000000000000000000000000000000000001);
        assert_equal(1, 1000000000000000000000000000000000000001 <=> 1000000000000000000000000000000000000000);
        assert_equal(1, 1000000000000000000000000000000000000001 <=> 1);
    }
    
    def test_or {
        assert_equal(1000000000000000000000000000000000000001, 1000000000000000000000000000000000000000 | 1);
    }
    
    def test_and {
        assert_equal(1, 1000000000000000000000000000000000000001 & 1);
    }
    
    def test_xor {
        assert_equal(1000000000000000000000000000000000000001, 1000000000000000000000000000000000000002 ^ 3);
    }
}.register;
