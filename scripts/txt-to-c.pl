print "char* " . shift . " = ";
while(<>) {
    chomp;
    s/\\/\\\\/g;
    s/"/\\"/g;
    print "\"";
    print;
    print "\\n\"\n";
}
print ";\n";