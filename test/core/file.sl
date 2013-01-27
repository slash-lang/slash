<%

class FileTest extends Test {
    def test_open_read_write {
        tmp = ENV["TMP"] || "/tmp";
        filename = tmp + "/slash-test-" + rand().to_s;
        
        f = File.new(filename, "w");
        f.write("hello world");
        f.close;
        
        f = File.new(filename, "r");
        assert_equal("hello world", f.read);
        f.close;
    }
    
    def test_closed {
        f = File.new("test.sl", "r");
        assert_equal(false, f.closed);
        f.close;
        assert_equal(true, f.closed);
    }
}.register;
