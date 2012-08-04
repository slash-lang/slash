Dir["**/*.{c,h}"].each do |f|
  if File.read(f)[-1] != "\n"
    puts f
    File.write f, File.read(f) + "\n"
  end
end