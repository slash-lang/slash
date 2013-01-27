<%

class FileTest extends Test {
    def temp_filename {
        (ENV["TMP"] || "/tmp") + "/slash-test-" + rand().to_s;
    }

    def test_open_read_write {
        filename = temp_filename();
        
        f = File.new(filename, "w");
        f.write("hello world");
        f.close;
        
        f = File.new(filename, "r");
        assert_equal("hello world", f.read);
        f.close;
    }
    
    def test_closed {
        f = File.new(temp_filename(), "w");
        assert_equal(false, f.closed);
        f.close;
        assert_equal(true, f.closed);
    }
}.register;
