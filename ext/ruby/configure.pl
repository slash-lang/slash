my $ruby = `which ruby`;
chomp $ruby;

my $ruby_libdir = `$ruby -e 'print RbConfig::CONFIG["libdir"]'`;
my $ruby_incdir = `$ruby -e 'print RbConfig::CONFIG["rubyhdrdir"]'`;
my $ruby_archincdir = `$ruby -e 'print RbConfig::CONFIG["rubyarchhdrdir"]'`;
my $ruby_libruby_a = `$ruby -e 'print RbConfig::CONFIG["LIBRUBY_A"]'`;

$LDFLAGS .= " $ruby_libdir/$ruby_libruby_a";
$CFLAGS  .= " -I$ruby_incdir -I$ruby_archincdir";

check_lib "ruby-static", <<C, skip_ldflag => 1;
    #include <ruby/version.h>
    
    #if RUBY_API_VERSION_MAJOR != 2 || RUBY_API_VERSION_MINOR != 0
        #error not using ruby 2.0
    #endif

    int main() {
    }
C

needs_static_init;
