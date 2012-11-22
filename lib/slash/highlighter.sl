<%

class Highlighter {
    RULES = [
        [ "whitespace", %r{^\s+} ],
        [ "comment",    %r{^#(.*?)\n} ],
        [ "comment",    %r{^//(.*?)\n} ],
        [ "comment",    %r{^/\*((.|\n)*?)\*/} ],
        [ "string",     %r{^"((\\\n|\\(.|\n)|((?!")(?!\\)(.|\n)))*?((?!")(?!\\)(.|\n))?)"} ],
        [ "regexp",     %r{^%r\{(.*?)\}} ],
        [ "number",     %r{^-?[0-9]+e[-+]?[0-9]+} ],
        [ "number",     %r{^-?[0-9]+(\.[0-9]+)(e[+-]?[0-9]+)?} ],
        [ "number",     %r{^-?[0-9]+} ],
        [ "constant",   %r{^[A-Z][a-zA-Z0-9_]*} ],
        [ "ivar",       %r{^@[a-z_][a-zA-Z0-9_]*} ],
        [ "cvar",       %r{^@@[a-z_][a-zA-Z0-9_]*} ],
    ];
    
    for kw in [ "nil", "true", "false", "self" ] {
        RULES.push([ "value", Regexp.new("^" + kw + "(?![a-z0-9A-Z_])")]);
    }
    
    for kw in [ "class", "extends", "def", "if", "elsif", "else", "unless",
                "for", "in", "while", "until", "and", "or", "not", "lambda",
                "try", "catch", "return", "next", "last", "throw"] {
        RULES.push(["keyword", Regexp.new("^" + kw + "(?![a-z0-9A-Z_])")]);
    }

    RULES.push([ "identifier", %r{^[a-z_][a-zA-Z0-9_]*} ]);
    
    STYLES = {
        "string"    => "color:#BA2121;",
        "regexp"    => "color:#BB6688;",
        "number"    => "color:#666666;",
        "constant"  => "color:#880000;",
        "ivar"      => "color:#19177C;",
        "cvar"      => "color:#19177C;",
        "keyword"   => "color:green; font-weight:bold;",
        "value"     => "color:green;",
        "comment"   => "color:#408080; font-style:italic;",
        "_default"  => "color:#111111;"
    };
    
    def init(source) {
        @source = source;
        self.lex;
    }
    
    def source { @source }
    def tokens { @tokens }
        
    def lex {
        @tokens = [];
        str = @source;
        until str.length == 0 {
            matched = false;
            for token, regexp in RULES {
                if md = regexp.match(str) {
                    @tokens.push([token, md[0]]);
                    str = md.after;
                    matched = true;
                    last;
                }
            }
            unless matched {
                if @tokens[-1] && @tokens[-1][0] == "unknown" {
                    @tokens[-1][1] += str[0];
                } else {
                    @tokens.push(["unknown", str[0]]);
                }
                str = str.split("").drop(1).join; # TODO: implement string slicing...
            }
        }
        @tokens;
    }
    
    def to_html(style_override = {}) {
        styles = STYLES.merge(style_override);
        html = "";
        for type, content in @tokens {
            style = styles[type] || styles["_default"];
            html += "<span style='" + style.html_escape + "'>" + content.html_escape + "</span>";
        }
        html;
    }
}