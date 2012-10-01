#!/usr/bin/env ruby

test_dir = File.dirname __FILE__

exec "#{test_dir}/../sapi/cli/slash-cli", "#{test_dir}/test.sl", *Dir["#{test_dir}/*/*.sl"]