# CGI/FCGI SAPI for Slash

This SAPI enables the use of Slash in a CGI or FastCGI environment.

At the moment it's probably not quite production-ready. Use with caution.

## Requirements

The only requirement for using slash-cgi is the "FastCGI developmen kit".

For *Debian* (and probably *Ubuntu*): 

    sudo apt-get install libfcgi-dev

For CentOS/Fedora, you have get it from the EPEL software repsoitory. The
easiest way to do this, is by [installing the `epel-release`-package](http://fedoraproject.org/wiki/EPEL/FAQ#How_can_I_install_the_packages_from_the_EPEL_software_repository.3F)
first. Then you can use `yum` to install the EPEL-packages:

    sudo yum install fcgi fcgi-devel

On MacOS you can use `brew` to install the FastCGI development kit:

    brew install fcgi

If all else fails, you have to get the development kit from
[fastcgi.com](http://www.fastcgi.com/drupal/node/5) and build it manually.

## Installation and configuration

To build slash-cgi you just add `--sapi=cgi` to your `./configure`-line:

    ./configure --sapi=cgi # ... and maybe other parameters
    # then build it as usual:
    make
    make install

The exact configuration process depends on the Webserver used. The SAPI tries
to emulate the behaviour of PHP-cgi, so most of the time the Instructions for
PHP should also work for slash-cgi.

The following sections are providing a simple overview of how to configure
your webserver to work with slash-cgi.

### Apache & mod_fcgid/mod_fastcgi via fcgid-script/fastcgi-script handler

This setup does not require slash-cgi to be executed as daemon. The webserver
instead runs a FCGI-script which calls slash-cgi. Slash-cgi then runs as
background-process and it stays in memory as long as apache runs (or apache
decides to terminate the process).

This type of configuration doesn't require you to have access to the 
global apache config files (given the modules mod_fcgid or mod_fastcgi are
already installed), the `AddHandler` and `Action` directives can be used in
`.htaccess`-files.

Add the following to your apache configuration (or to a .htaccess):

    AddHandler fcgid-script .fcgi
    AddHandler slash-cgi .sl
    Action slash-cgi /cgi-bin/slash.fcgi

Then in your FCGI-script (`cgi-bin/slash.fcgi`):

    #!/bin/sh

    # replace this path with the actual path to slash-cgi
    exec /usr/local/bin/slash-cgi

### Apache & mod_fastcgi via socket

With mod_fastcgi you have to run slash-cgi as daemon and tell it to listen
to a UNIX- or TCP/IP-socket. This type of installation requires you to have
access to the global apache configuration.

You can use the [`FastCgiExternalServer`](http://www.fastcgi.com/mod_fastcgi/docs/mod_fastcgi.html#FastCgiExternalServer)
directive to configure a FastCGI endpoint.

    # This maps an action-type to a web-path
    Action slash-ext-fcgi /slash-ext-fcgi

    # Associathe the .sl file extension with the action-type
    AddHandler slash-ext-fcgi .sl

    # Now, forward the web-path to a virtual file in the filesystem
    # the file does not have to exists, but the directory structure
    # must be valid 
    Alias /slash-ext-fcgi /tmp/slash-cgi

    # Now tie the virtual file to a FastCGI endpoint

    # Note: if `FastCgiWrapper` is set to `off` you don't need to
    # specify the user and group parameters.

    # either using an unix socket
    FastCgiExternalServer /tmp/slash-cgi -socket /tmp/slash-ext-fcgi.sock -pass-header Authorization -user youruser -group yourgroup

    # or using a TCP/IP socket:
    FastCgiExternalServer /tmp/slash-cgi -host localhost:1234 -pass-header Authorization -user youruser -group yourgroup

### Nginx

With nginx you have to run slash-cgi as daemon. The configuration is somewhat
straight-forward:

    location ~ \.sl(/.*)?$ {
        # using an unix socket:
        fastcgi_pass            unix:/path/to/slash-cgi.sock;
        # or using a TCP/IP socket:
        fastcgi_pass            localhost:1234;

        fastcgi_split_path_info ^(.+\.sl)(/.*)$;

        fastcgi_param           PATH_INFO       $fastcgi_path_info;
        fastcgi_param           PATH_TRANSLATED $document_root$fastcgi_path_info;
        fastcgi_param           SCRIPT_FILENAME $document_root$fastcgi_script_name;

        include                 fastcgi_params;
    }


### CGI

If your webserver does not support FastCGI but CGI you can use slash-cgi in
the CGI-mode. Just add a Hashbang/Shebang line to your script:

    #!/usr/local/bin/slash-cgi
    <% print("Hello ", Request.remote_addr, " from CGI");

The shebang line is ignored by slash-cgi so it won't appear in the output.

You can put that file in your cgi-bin folder and, if your webserver requires it,
set the executable bit.

## Run slash-cgi as FCGI daemon

Well basically you just have to call `slash-cgi` with the `-b`-parameter. You
can use the following formats:

    # bind sapi-cgi to an UNIX socket:
    sapi-cgi -b /path/to/unix-socket.sock

    # bind to a TCP/IP socket:

    # listens on localhost:1234 (this is the only supporter format on Windows)
    sapi-cgi -b :1234

    # listens on a specific interface at port 1234:
    sapi-cgi -b 192.168.0.2:1234

Currently there are no init-scripts provided. You probably have to build
something yourself (contributions are, of course, welcome). For simple
use cases you can use `nohup`. Sorry.

## TODO

* Test it (like, a **lot**)
    * Especially the part which calculates the correct `SCRIPT_FILENAME` to
      use, since this varies between different webservers/setups.
    * Test it on Windows (maybe even with IIS)
* Add Support for pre-forked children
    * Also allow to set a maximum number of requests a children handles before
      it dies (and makes place for a "fresh" children)
    * Throw in some init-files for Debian/CentOOS and maybe other plattforms
* Clean up the code
    * There are some TODO items spread across the code
    * Additionally there are some inconsistencies in the code (naming,
      code-style).
* About the FastCGI dev-kit:
    * Maybe ship it with the SAPI?
    * Make it optional, so slash-cgi can be compiled as CGI-only if the dev-kit
      is not installed?
    * Are there alternative FastCGI-bindings?
* Improve this readme
