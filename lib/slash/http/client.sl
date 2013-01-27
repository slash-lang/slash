<%

use URI;

class HTTP {
    class Client {
        class ProtocolError extends Error {}
        
        def init(uri) {
            uri = URI.parse(uri) unless uri.is_a(URI);
            @uri = uri;
            @headers = {
                "User-Agent" => "Slash HTTP::Client"
            };
        }
        
        def get    { perform("GET") }
        def head   { perform("HEAD") }
        def post   { perform("POST") }
        def put    { perform("PUT") }
        def delete { perform("DELETE") }
            
        def response_headers { @response_headers }
        def response_body    { @response_body }
        def status           { @status }
        
        def perform(method) {
            throw ArgumentError.new("Invalid method") if %r{[^A-Z]}.match(method);
            path = @uri.path || "/";
            throw ArgumentError.new("Path contains whitespace") if %r{\s}.match(path);
            query = @uri.query || "";
            throw ArgumentError.new("Query string contains whitespace") if %r{\s}.match(query);
            headers = @headers.merge({
                "Connection" => "close",
                "Host" => @uri.host,
            }).map(\(k, v) {
                k = k.to_s;
                v = v.to_s;
                throw ArgumentError.new("Invalid header name: " + k.inspect) if %r{[\r\n:]}.match(k);
                throw ArgumentError.new("Invalid header value: " + v.inspect) if %r{[\r\n]}.match(v);
                k + ": " + v + "\r\n";
            }).join;
            socket = TCPSocket.new(@uri.host, @uri.port);
            socket.write(method + " " + path + query + " HTTP/1.1\r\n" + headers + "\r\n");
            @response_headers = {};
            @response_body = "";
            read_response(socket);
            true;
        }
        
        def read_response(socket) {
            throw ProtocolError.new unless md = %r{^HTTP\d/\d (?<status>\d+)}.match(socket.read_line);
            @status = md["status"].to_i;
            while true {
                line = socket.read_line;
                last if line == "\r\n";
                throw ProtocolError.new unless md = %r{^\s*(?<name>.*?):\s*(?<value>.*?)\s*$}.match(line);
                if @response_headers[md["name"]].is_a(Array) {
                    @response_headers[md["name"]].push(md["value"]);
                } elsif @response_headers[md["name"]] {
                    @response_headers[md["name"]] = [@response_headers[md["name"]], md["value"]];
                } else {
                    @response_headers[md["name"]] = md["value"];
                }
            }
            while buff = socket.read(4096) {
                @response_body += buff;
            }
        }
    }
}
