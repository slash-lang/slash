<%

class TimeTest extends Test {
    def test_to_i {
        assert_equal(946684800, Time.new(2000, 1, 1, 0, 0, 0).to_i);
    }
    
    def test_inspect {
        assert_equal("Sat Jan 01 2000 00:00:00 UTC", Time.new(2000, 1, 1, 0, 0, 0).inspect);
    }
    
    def test_equality {
        assert_equal(Time.new(2000, 1, 1, 0, 0, 0), Time.new(2000, 1, 1, 0, 0, 0));
    }
    
    def test_inequality {
        assert_unequal(Time.new(2000, 1, 1, 0, 0, 0), Time.new(1999, 12, 31, 23, 59, 59));
    }
    
    def test_cmp {
        assert_equal(1, Time.new(2000, 1, 1, 0, 0, 0) <=> Time.new(1999, 12, 31, 23, 59, 59));
        assert_equal(-1, Time.new(1999, 12, 31, 23, 59, 59) <=> Time.new(2000, 1, 1, 0, 0, 0));
        assert_equal(0, Time.new(1999, 12, 31, 23, 59, 59) <=> Time.new(1999, 12, 31, 23, 59, 59));
    }
    
    def test_add {
        assert_equal(Time.new(2000, 1, 1, 0, 0, 0), Time.new(1999, 12, 31, 23, 59, 59) + 1);
    }
    
    def test_sub {
        assert_equal(Time.new(1999, 12, 31, 23, 59, 59), Time.new(2000, 1, 1, 0, 0, 0) - 1);
        assert_equal(1, Time.new(2000, 1, 1, 0, 0, 0) - Time.new(1999, 12, 31, 23, 59, 59));
    }
    
    def test_strftime {
        assert_equal("Saturday", Time.new(2000, 1, 1, 0, 0, 0).strftime("%A"));
    }
    
    def test_strftime_with_resize {
        padding = ".";
        for i in 1..10 {
            padding = padding + padding;
        }
        assert_equal(padding + "Saturday" + padding, Time.new(2000, 1, 1, 0, 0, 0).strftime(padding + "%A" + padding));
    }
    
    def test_clock_is_monotonic {
        last_clock = 0;
        for i in 1..100 {
            next_clock = Time.clock;
            assert(next_clock > last_clock, "clock is always greater than last clock");
            last_clock = next_clock;
        }
    }
}.register;