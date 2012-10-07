needs_static_init;

if($^O eq "msys") {
    $LDFLAGS .= " -lWS2_32";
}