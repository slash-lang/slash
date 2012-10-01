#!/usr/bin/env ruby
require "tempfile"

test_dir = File.dirname __FILE__

failures = []

puts "Running tests...\n"

Dir["#{test_dir}/**/*.sl"].each do |test|
  *, group, name = test.split "/"
  data = Hash[File.readlines(test).slice_before { |line| line =~ /@[a-z]+/i }.map { |header, *rest| [header.chomp, rest] }]
  Tempfile.open "#{group}-#{name}" do |f|
    f.write data["@CODE"].join
    f.close
    output = `#{test_dir}/../sapi/cli/slash-cli #{f.path} 2>&1`
    if data["@EXPECT"].join.strip == output.strip
      print "."
    else
      print "F"
      failures << [group, name, data["@EXPECT"], output]
    end
  end
end

puts
if failures.any?
  puts "\nFailures:"

  failures.each do |group, name, expected, actual|
    puts "- \e[1m#{group}/#{name}\e[0m"
    expected.each do |line|
      puts "  \e[32;1m#{line.chomp}\e[0m"
    end
    actual.each_line do |line|
      puts "  \e[33;1m#{line.chomp}\e[0m"
    end
  end
end