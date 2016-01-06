#!/usr/bin/ruby
if ARGV.length < 2
  puts "Usage: ruby sign_update.rb version private_key_file"
  puts "\nCall this from the build directory."
  puts "If you don't have the tomahawk private key and you think you should, ask leo :)"
  exit
end

tarball = "#{ARGV[0].downcase}-#{ARGV[1]}.tar.bz2"
puts "Zipping: #{tarball}..."
`tar jcvf "#{tarball}" #{ARGV[0]}.app`

puts "Signing..."
puts `openssl dgst -sha1 -binary < "#{tarball}" | openssl dgst -dss1 -sign "#{ARGV[2]}" | openssl enc -base64`
