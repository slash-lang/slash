$filename = shift;
$filename =~ qr{/([^/]*)$};
$symbol_name = $1;
$symbol_name =~ s/[^a-zA-Z_]/_/g;
print "char* sl__$symbol_name = ";
open FH, $filename;
while(<FH>) {
    chomp;
    s/\\/\\\\/g;
    s/"/\\"/g;
    print "\"";
    print;
    print "\\n\"\n";
}
print ";\n";