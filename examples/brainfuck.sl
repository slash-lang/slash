<%

class Env {
    def init {
        @memory = [];
        @ptr = 0;
    }

    def ptr       { @ptr }
    def ptr=(ptr) { @ptr = ptr }

    def current_value         { @memory[@ptr] || 0 }
    def current_value=(value) { @memory[@ptr] = value }
}

class AST {
    class Next {
        def eval(env) {
            env.ptr++;
        }
    }

    class Prev {
        def eval(env) {
            env.ptr--;
        }
    }

    class Inc {
        def eval(env) {
            env.current_value++;
        }
    }

    class Dec {
        def eval(env) {
            env.current_value--;
        }
    }

    class Output {
        def eval(env) {
            print(env.current_value.char);
        }
    }

    class Input {
        def eval(env) {
            ...
        }
    }

    class Sequence {
        def init(nodes) {
            @nodes = nodes;
        }

        def eval(env) {
            for node in @nodes {
                node.eval(env);
            }
        }
    }

    class Loop {
        def init(seq) {
            @seq = seq;
        }

        def eval(env) {
            while env.current_value != 0 {
                @seq.eval(env);
            }
        }
    }
}

class Parser {
    def init(str) {
        @chars = str.split("");
    }

    def parse {
        @stack = [[]];
        for char in @chars {
            _parse_char(char);
        }
        if @stack.length != 1 {
            throw SyntaxError.new("unexpected end of input");
        }
        AST::Sequence.new(@stack.last);
    }

    def _parse_char(char) {
        if char == ">" {
            _add(AST::Next.new);
        } elsif char == "<" {
            _add(AST::Prev.new);
        } elsif char == "+" {
            _add(AST::Inc.new);
        } elsif char == "-" {
            _add(AST::Dec.new);
        } elsif char == "." {
            _add(AST::Output.new);
        } elsif char == "," {
            _add(AST::Input.new);
        } elsif char == "[" {
            _open_loop();
        } elsif char == "]" {
            _close_loop();
        }
    }

    def _add(node) {
        @stack.last.push(node);
    }

    def _open_loop {
        @stack.push([]);
    }

    def _close_loop {
        if @stack.length == 1 {
            throw SyntaxError.new("unexpected ']'");
        }

        nodes = @stack.pop;
        _add(AST::Loop.new(AST::Sequence.new(nodes)));
    }
}

src = File.read(ARGV.first);
ast = Parser.new(src).parse;
ast.eval(Env.new);
