# apache2 SAPI

Once the apache2 SAPI is loaded into Apache, you can execute files as Slash scripts with the `slash` handler.

For example, to designate all files with the `.sl` extension as Slash scripts, insert the following into your `httpd.conf` or `.htaccess`:

```apache
AddHandler slash .sl
```

## Mac OS X 10.8+

Mac OS X since 10.8 has broken Apache module compilation.

If you're seeing an error along the lines of:

```
/usr/share/apr-1/build-1/libtool: line 4574: /Applications/Xcode.app/Contents/Developer/Toolchains/OSX10.9.xctoolchain/usr/bin/cc: No such file or directory
```

Then run this command (replacing `OSX10.9` with your version of OS X):

```sh
sudo ln -s /Applications/Xcode.app/Contents/Developer/Toolchains/{XcodeDefault.xctoolchain,OSX10.9.xctoolchain}
```
