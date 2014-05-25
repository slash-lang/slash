<%

use Test;

if ARGV.first == "--gc-after-test" {
    Test.gc_after_test = true;
    ARGV.shift;
}

# pass this via argument
# TODO: calculate this in slash as soon as the 
# necessary functions (realpath) are available
SAPI_CGI_BASE_PATH=ARGV.shift;

for file in ARGV {
    require(file);
}

Test.run_suite;
Test.print_report;
exit(1) if Test.any_failures;
