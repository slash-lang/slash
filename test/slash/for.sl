<%

class ForTest extends Test {
    def test_for {
        x = 0;
        for i in 1..3 {
            x += i;
        }
        assert_equal(x, 6);
    }
    
    def test_multiple_assignment_in_for {
        for a, b in [[1, 2]] {
            assert_equal(1, a);
            assert_equal(2, b);
        }
    }
    
    def test_for_else {
        pass = false;
        for i in [] {
            flunk();
        } else {
            pass = true;
        }
        assert(pass);
        
        pass = false;
        for i in [1] {
            pass = true;
        } else {
            flunk();
        }
        assert(pass);
    }
}.register;