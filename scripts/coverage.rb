#!/usr/bin/env ruby
require "fileutils"

def cmd(what)
  retn = IO.popen "#{what} 2>&1", "rb"
  Process.wait retn.pid
  if $?.success?
    retn
  else
    puts "\e[31;1m!!!!\e[0m '#{what}' failed"
    exit 1
  end
end

Dir.chdir "#{__FILE__}/../.."

sources = Dir["src/**/*.c"]

cmd "make clean"
cmd "./configure --vanilla --coverage --debug --prefix=`pwd` --sapi=cli"
cmd "make -j 8"

cmd "make test"

total_lines = 0
total_missed_lines = 0

sources.each do |source|
  if source.include? "lib"
    cmd "gcov -o src/lib ../#{source}"
  else
    cmd "gcov -o src ../#{source}"
  end
  gcov_file = "#{source.split("/").last}.gcov"
  cov_lines = File.readlines(gcov_file).reject { |x| x =~ /^\s*-:/ }
  missed = cov_lines.select { |x| x =~ /^\s*#/ and not x.include? "/* never reached */" }
  total_lines += cov_lines.size
  total_missed_lines += missed.size
  coverage = 100 - (missed.size/cov_lines.size.to_f) * 100
  if coverage < 75
    printf "\e[31;1m%s - %2d%%\e[0m\n", source, coverage
  elsif coverage < 90
    printf "\e[33;1m%s - %2d%%\e[0m\n", source, coverage
  else
    printf "\e[32;1m%s - %2d%%\e[0m\n", source, coverage
  end
  missed.each do |line|
    puts "    #{line}"
  end #if missed.size < 20
  #require "pry"
  #pry binding
  #puts "\e[33;1mCoverage report for #{source}\e[0m"
end

cmd "make clean"

puts
puts
total_coverage = 100 - (total_missed_lines/total_lines.to_f) * 100
if total_coverage < 75
  printf "\e[31;1m** Total coverage: %2d%%\e[0m\n", total_coverage
elsif total_coverage < 90
  printf "\e[33;1m** Total coverage: - %2d%%\e[0m\n", total_coverage
else
  printf "\e[32;1m** Total coverage: - %2d%%\e[0m\n", total_coverage
end

puts

exit