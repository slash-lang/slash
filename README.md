# Slash

[![Travis Status](https://secure.travis-ci.org/slash-lang/slash.png)](http://travis-ci.org/slash-lang/slash)

Slash is a new language for the web. It's designed for the cases when you just want to throw up a simple dynamic page and don't want to have to bother setting up and maintaining application servers.

It's inspired by Ruby, Perl and good old PHP.

## Supported Platforms

* Linux - x86 and x86_64
* Mac OS X - x86_64
* Windows - x86

## Installing

### Sane systems

    $ ./configure
    $ make
    # make install

By default, `./configure` will enable all extensions (`mysql`, `json`, `base64`, `gcrypt` and `inflect` at time of writing). To disable an extension, use the `--no-ext=<extname>` option. To enable a disabled extension, use `--ext=<extname>`.

Slash by itself is compiled as a static library (`libslash.a` on *nix systems). To actually do anything useful with Slash, you must compile a SAPI (Server API). The SAPI is the piece of software that glues libslash to your web server. For example, if you wish to run Slash on the Apache Web Server, you must enable the `apache2` SAPI by passing `--sapi=apache2` to `./configure`. By default, the `cli` SAPI is automatically enabled. The `cli` SAPI allows you to run Slash scripts from the command line.

If you have libraries installed in non standard locations and the configure script fails to find them, you may specify the location manually with the `--with-lib-dir=<path>` option.

If Slash *still* fails to configure, you may use the `--verbose` switch to log additional information to the terminal. This information will be required if you need to report an issue.

`make install` will install Slash under `/usr/local` by default. This install prefix can be changed with the `--prefix=<path>` option.

#### OS X

If you have [Homebrew](http://mxcl.github.io/homebrew/) installed you can install the dependencies by running:

    brew install gmp pcre libgcrypt yajl discount



### Windows

A vanilla Slash can be built on Windows using mingw-gcc. In the future, binaries for `slash-cli` and common SAPIs will be distributed.