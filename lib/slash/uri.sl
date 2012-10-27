<%

class URI {
    def init(scheme = nil, auth = nil, host = nil, port = nil, path = nil, query = nil, fragment = nil) {
        scheme  ||= "http";
        port    ||= 80;
        path    ||= "/";
        
        [@scheme, @auth, @host, @port, @path, @query, @fragment] =
        [scheme,  auth,  host,  port,  path,  query,  fragment];
    }
    
    def self.parse(uri) {
        md = %r{^(?<scheme>[a-z]+)://((?<auth>.*?)@)?((?<host>[a-z0-9.]+)(:(?<port>\d+))?)((?<path>/.*?)((?<query>\?.*)(?<fragment>#.*)?)?)$}.match(uri);
        new(md["scheme"], md["auth"], md["host"], md["port"], md["path"], md["query"], md["fragment"]);
    }
    
    def scheme   { @scheme }
    def auth     { @auth }
    def host     { @host }
    def port     { @port }
    def path     { @path }
    def query    { @query }
    def fragment { @fragment }
        
    def scheme=(val)   { @scheme = val; }
    def auth=(val)     { @auth = val; }
    def host=(val)     { @host = val; }
    def port=(val)     { @port = val; }
    def path=(val)     { @path = val; }
    def query=(val)    { @query = val; }
    def fragment=(val) { @fragment = val; }
        
    def to_s {
        if @scheme and @scheme != "" {
            url = @scheme;
        } else {
            url = "http";
        }
        url += "://";
        if @auth and @auth != "" {
            url += @auth;
            url += "@";
        }
        url += @host;
        if @port and @port != "" {
            url += ":" + @port;
        }
        url += @path + @query + @fragment;
    }
}
