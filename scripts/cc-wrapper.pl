#!/usr/bin/env perl
foreach(`gcc @ARGV 2>&1`) {
    if(/error/) {
        print "\e[31;1m$_\e[0m";
    } elsif(/warning/) {
        print "\e[33;1m$_\e[0m";
    } else {
        print;
    }
}