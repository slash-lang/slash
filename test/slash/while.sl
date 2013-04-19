<%

class WhileTest extends Test {
    def test_while {
        while false {
            flunk();
        }
        
        pass = true;
        while pass {
            pass = false;
        }
    }
    
    def test_until {
        until true {
            flunk();
        }
        
        pass = false;
        until pass {
            pass = true;
        }
    }
}