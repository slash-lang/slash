<%

class DictTest extends Test {
    def test_class {
        assert_is_a(Dict, {});
        assert_is_a(Dict, Dict.new);
    }
    
    def test_get {
        assert_equal(nil, {}["hello"]);
        assert_equal(123, { "hello" => 123 }["hello"]);
        assert_equal(456, { "hello" => 123, "hello" => 456 }["hello"]);
    }
    
    def test_set {
        d = {};
        assert_equal(nil, d["x"]);
        d["x"] = 123;
        assert_equal(123, d["x"]);
    }
    
    def test_length {
        assert_equal(0, {}.length);
        assert_equal(2, { "a" => 1, "b" => 2 }.length);
        assert_equal(1, { "a" => 1, "a" => 2 }.length);
        d = {};
        assert_equal(0, d.length);
        d["x"] = 123;
        assert_equal(1, d.length);
    }
    
    def test_delete {
        d = { "x" => 123 };
        assert_equal(123, d["x"]);
        d.delete("x");
        assert_equal(nil, d["x"]);
        assert(d.empty, "dictionary expected to be empty");
    }
    
    def test_merge {
        assert_equal({ "a" => 2 }, { "a" => 1 }.merge({ "a" => 2 }));
        assert_equal({ "a" => 1, "b" => 2 }, { "a" => 1 }.merge({ "b" => 2 }));
        assert_equal({ "a" => 1 }, { "a" => 1 }.merge({}));
        assert_equal({ "a" => 1 }, {}.merge({ "a" => 1 }));
    }
    
    def test_enumerate {
        dict = { "a" => 1 };
        enumerator = dict.enumerate;
        assert_equal(true, enumerator.next);
        assert_equal(["a", 1], enumerator.current);
        assert_equal(false, enumerator.next);
    }
    
    def test_to_s {
        assert_equal("{ \"a\" => 1 }", { "a" => 1 }.to_s);
        assert_equal("{ \"a\" => 1 }", { "a" => 1 }.inspect);
    }
    
    def test_recursive_to_s {
        d = {};
        d[d] = d;
        assert_equal("{ { <recursive> } => { <recursive> } }", d.to_s);
        
        d = {};
        d[d] = 1;
        assert_equal("{ { <recursive> } => 1 }", d.to_s);
        
        d = {};
        d[1] = d;
        assert_equal("{ 1 => { <recursive> } }", d.to_s);
    }
    
    def test_eq {
        assert_equal(false, { "a" => 1 } == {});
        assert_equal(false, {} == { "a" => 1 });
        assert_equal(false, { "a" => 1 } == { "a" => 2 });
        assert_equal(true, { "a" => 1 } == { "a" => 1 });
        
        always_equal = Object.new;
        def always_equal.==(other) { true }
        assert_equal(true, { "a" => always_equal } == { "a" => 1 });
    }
}.register;