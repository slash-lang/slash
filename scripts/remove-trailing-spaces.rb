Dir["**/*.{c,h,inc,pl,mk,yy,sl}"].each do |f|
  if File.readlines(f).any? { |line| line =~ /([^ ] +)$/ }
    puts f
  end
end