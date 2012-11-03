<%

class IfTest extends Test {
    def test_if {
        pass = false;
        if true {
            pass = true;
        }
        assert(pass);
        
        pass = true;
        if false {
            pass = false;
        }
        assert(pass);
    }
    
    def test_elsif {
        pass = false;
        if false {
            flunk();
        } elsif true {
            pass = true;
        } elsif true {
            flunk();
        } else {
            flunk();
        }
        assert(pass);
    }
    
    def test_unless {
        pass = false;
        unless false {
            pass = true;
        }
        assert(pass);
        
        pass = true;
        unless true {
            pass = false;
        }
        assert(pass);
    }
    
    def test_postfix_if {
        pass = false;
        pass = true if true;
        assert(pass);
        flunk() if false;
    }
    
    def test_postfix_unless {
        pass = false;
        pass = true unless false;
        assert(pass);
        flunk() unless true;
    }
}.register;