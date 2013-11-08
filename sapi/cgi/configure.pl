print "Checking for libfcgi()... ";
if(compile("-lfcgi", "#include <fcgiapp.h>\nint main(){FCGX_IsCGI();}")) {
    print "ok\n";
    $LDFLAGS .= " -lfcgi";
    $CFLAGS .= " -DSL_HAS_LIBFCGI";
} else {
    print "failed\n";
}
