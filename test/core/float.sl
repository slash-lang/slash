<%

class FloatTest extends Test {
    def test_succ {
        assert_equal(2.5, 1.5.succ);
        assert_equal(-0.5, -1.5.succ);
    }
    
    def test_pred {
        assert_equal(1.5, 2.5.pred);
        assert_equal(-0.5, 0.5.pred);
    }
    
    def test_negate {
        assert_equal(-5.5, 5.5.negate);
        assert_equal(-(5.5), 5.5.negate);
    }
    
    def test_addition {
        assert_equal(124.0, 100.5 + 23.5);
        assert_equal(101.5, 100.5 + 1);
        assert_equal(123.0, 123.0 + Bignum.new);
        assert_is_a(Float, 123.0 + Bignum.new);
    }
    
    def test_subtraction {
        assert_equal(77, 100.5 - 23.5);
        assert_equal(77.0, 77.0 - Bignum.new);
        assert_equal(77.0, 78.0 - 1);
    }
    
    def test_multiplication {
        assert_equal(150.0, 100 * 1.5);
        assert_equal(0.0, 100.0 * Bignum.new);
        assert_is_a(Float, 100.0 * Bignum.new);
        assert_equal(300.0, 150.0 * 2);
        assert_is_a(Float, 150.0 * 2);
    }
    
    def test_division {
        assert_equal(2.5, 100 / 40.0);
        assert_equal(2.5, 100.0 / 40.0);
        assert_equal(Infinity, 100.0 / Bignum.new);
        assert_is_a(Float, 100.0 / Bignum.new);
        assert_equal(12.5, 25.0 / 2);
    }
    
    def test_modulo {
        assert_equal(0.5, 10.0 % 9.5);
        assert_equal(1234.5, 1234.5 % 9999999999999999999999999999999999999999);
        assert_equal(0.5, 1234.5 % 1234);
    }
    
    def test_power {
        assert_equal(2.0, 4.0 ** 0.5);
        assert_equal(16.0, 4.0 ** 2);
        assert_equal(1.0, 4.0 ** Bignum.new);
    }
    
    def test_to_s_and_inspect {
        assert_equal("5.5", 5.5.to_s);
        assert_equal("5.5", 5.5.inspect);
    }
    
    def test_to_i {
        assert_equal(100, 100.5.to_i);
        assert_is_a(Int, 100.5.to_i);
        assert_is_a(Bignum, 9999999999999999999999999999999999999999.9.to_i);
    }
    
    def test_to_f {
        assert_equal(100.5, 100.5.to_f);
        assert_is_a(Float, 100.5.to_f);
    }
    
    def test_equality {
        assert_equal(100.0, 100);
        assert_equal(0.0, Bignum.new);
    }
    
    def test_inequality {
        assert(100.5 != 100.5001, "Expected 100.5 to not equal 100.5001");
        assert_unequal(0.5, Bignum.new);
        assert_unequal(0.5, nil);
    }
    
    def test_spaceship {
        assert_equal(0, 100.5 <=> 100.5);
        assert_equal(-1, 100.500000 <=> 100.500001);
        assert_equal(1, 100.500001 <=> 100.500000);
        assert_equal(1, Infinity <=> 9999999999999999999999999999999999999999);
        assert_equal(-1, 0.5555 <=> 9999999999999999999999999999999999999999)
    }
    
    def test_class {
        assert_is_a(Float, 123.0);
    }
}.register;
