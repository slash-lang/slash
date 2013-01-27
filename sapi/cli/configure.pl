print "Checking for readline()... ";
if(compile("-lreadline", "#include <readline/readline.h>\nint main(){readline(\"\");}")) {
    print "ok\n";
    $LDFLAGS .= " -lreadline";
    $CFLAGS .= " -DSL_HAS_READLINE";
} else {
    print "failed\n";
}
