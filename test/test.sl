<%

class Test {
    FAILURES = [];
    CASES = [];
    
    class Failure extends Error {
    }
    
    def assert(what, message) {
        unless what {
            throw Failure.new(message);
        }
    }
    
    def assert_equal(expect, what) {
        assert(expect == what, "Expected " + expect.inspect + ", got " + what.inspect);
    }
    
    def assert_is_a(klass, what) {
        assert(what.is_a(klass), "Expected " + what.inspect + " to be a " + klass.name);
    }
    
    def self.run {
        obj = new();
        for method in obj.own_methods {
            next unless md = %r{^test_(.*)$}.match(method);
            try {
                obj.send(method);
                print(".");
            } catch e {
                print("F");
                FAILURES.push([name() + " " + md[1], e]);
            }
        }
    }
    
    def self.register {
        CASES.push(self);
    }
}

for file in ARGV {
    require(file);
}

for test in Test::CASES {
    test.run;
}

print("\n\n");

for test_name, failure in Test::FAILURES {
    print(failure.name, " in ", test_name, " - ", failure.message, "\n");
    for frame in failure.backtrace {
        print("    ", frame, "\n");
    }
    print("\n");
}

exit(1) if Test::FAILURES.any;