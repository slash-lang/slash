<%

class RequestInfoTest extends Test {
    def before {
        @defdocroot = SAPI_CGI_BASE_PATH + "/test/runner/docroot";
        @defaultenv = {
            "HTTP_HOST" => "example.com",
            "HTTP_USER_AGENT" => "Mozilla/5.0 (Windows NT 6.2; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0",
            "HTTP_ACCEPT" => "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "HTTP_ACCEPT_LANGUAGE" => "de-de,de;q=0.8,en-us;q=0.5,en;q=0.3",
            "HTTP_ACCEPT_ENCODING" => "gzip, deflate",
            "HTTP_CONNECTION" => "keep-alive",
            "HTTP_COOKIE" => "__utma=laksdjfklasdfl; __utmz=1.jjjasdfjaslfjalsdjflaksdjflaksdjflasjdflaksdjflasjdlfkasdjflaksdjflasdjflaksdjfjj.1.utmcsr=(direct)|utmccn=(direct)|utmcmd=(none); cX_P=1381495017965570556784",

            "SERVER_SIGNATURE" => "<address>Apache/2.4.6 (Debian) Server at example.com Port 80</address>",
            "SERVER_SOFTWARE" => "Apache/2.4.6 (Debian)",
            "SERVER_NAME" => "example.com",
            "SERVER_ADDR" => "10.0.0.1",
            "SERVER_PORT" => "80",
            "REMOTE_ADDR" => "10.0.0.50",
            "REQUEST_SCHEME" => "http",
            "SERVER_ADMIN" => "webmaster@example.com",
            "REMOTE_PORT" => "64674",

            "GATEWAY_INTERFACE" => "CGI/1.1",
            "SERVER_PROTOCOL" => "HTTP/1.1",
            
            "DOCUMENT_ROOT"=> @defdocroot,
            "REQUEST_METHOD"=>"GET",
        };
    }

    def test_cgi {
        realdir = @defdocroot + "/cgi-bin";
        realfile = realdir + "/dummy.cgi";
        info = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_CGI,
            @defaultenv.merge({
                "QUERY_STRING"=>"",
                "REQUEST_URI"=>"/cgi-bin/dummy.cgi",
                "SCRIPT_NAME"=>"/cgi-bin/dummy.cgi",

                "SCRIPT_FILENAME" => realfile,
                "PATH_TRANSLATED" => @defdocroot + "/cgi-bin/dummy-invalid.cgi",
        }));

        assert_equal(realfile, info["real_canonical_filename"]);
        assert_equal(realdir, info["real_canonical_dir"]);
    }

    # In CGI-mode the PATH_TRANSLATED should be ignored
    def test_cgi_ignore_path_translated {
        info = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_CGI,
            @defaultenv.merge({
                "QUERY_STRING"=>"",
                "REQUEST_URI"=>"/dummy.sl",
                "SCRIPT_NAME"=>"/dummy.sl",

                "SCRIPT_FILENAME" => @defdocroot + "/dummy.sl",
                "PATH_TRANSLATED" => @defdocroot + "/dummy-invalid.sl",
        }));

        assert_equal(@defdocroot + "/dummy.sl", info["real_canonical_filename"]);
    }

    # If the file in PATH_TRANSLATED exists, it superseedes SCRIPT_FILENAME.
    # This is the case with Apache and mod_fcgid or mod_fascgi (when 
    # slash-cgi is executed as FCGI-action).
    def test_fastcgi_valid_path_translated {
        info = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_FCGI,
            @defaultenv.merge({
                "QUERY_STRING"=>"",
                "REQUEST_URI"=>"/dummy.sl",
                "SCRIPT_NAME"=>"/dummy.sl",

                "SCRIPT_FILENAME" => @defdocroot + "/dummy-invalid.sl",
                "PATH_TRANSLATED" => @defdocroot + "/dummy.sl",
        }));

        assert_equal(@defdocroot + "/dummy.sl", info["real_canonical_filename"]);
    }

    # if PATH_TRANSLATED is set but does not exist, fall back to
    # SCRIPT_FILENAME
    def test_fastcgi_invalid_path_translated {
        info = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_FCGI,
            @defaultenv.merge({
                "QUERY_STRING"=>"",
                "REQUEST_URI"=>"/dummy.sl",
                "SCRIPT_NAME"=>"/dummy.sl",

                "SCRIPT_FILENAME" => @defdocroot + "/dummy.sl",
                "PATH_TRANSLATED" => @defdocroot + "/not-existing.sl",
        }));

        assert_equal(@defdocroot + "/dummy.sl", info["real_canonical_filename"]);
    }

    # The environment shouldn't be altered
    def test_import_env_verbatim {
        the_env = @defaultenv.merge({
                "QUERY_STRING"=>"hey=world",
                "REQUEST_URI"=>"/dummy.sl?hey=world",
                "SCRIPT_NAME"=>"/dummy.sl",

                "SCRIPT_FILENAME" => @defdocroot + "/dummy.sl",
                "PATH_TRANSLATED" => @defdocroot + "/not-existing.sl",
        });

        info_fcgi = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_FCGI, the_env);

        assert_equal(the_env, info_fcgi["environment"]);

        info_cgi = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_CGI, the_env);

        assert_equal(the_env, info_cgi["environment"]);
    }

    # Are the HTTP_* variables correctly mapped to HTTP headers?
    def test_import_env_http_headers {
        http_header = {
            "Accept-Language" => "en",
            "Cookie" => "ab=cd; x=y",
            "Host" => "example-host.com",
        }

        env_http_header = {
            "HTTP_ACCEPT_LANGUAGE" => "en",
            "HTTP_COOKIE" => "ab=cd; x=y",
            "HTTP_HOST" => "example-host.com",
        }

        # the folliwng entries should not be mapped!
        the_env = env_http_header.merge({
            "HTTPS" => "on",
            "HTTPFUN" => "off",
        });
        
        info_cgi = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_CGI, env_http_header);

        assert_equal(http_header, info_cgi["http_headers"]);

        info_fcgi = CgiTest.load_request_info(
            CgiTest::SLASH_REQUEST_FCGI, env_http_header);

        assert_equal(http_header, info_fcgi["http_headers"]);
    }
}
