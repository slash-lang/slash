# apache2 SAPI

Once the apache2 SAPI is loaded into Apache, you can execute files as Slash scripts with the `slash` handler.

For example, to designate all files with the `.sl` extension as Slash scripts, insert the following into your `httpd.conf` or `.htaccess`:

```apache
AddHandler slash .sl
```