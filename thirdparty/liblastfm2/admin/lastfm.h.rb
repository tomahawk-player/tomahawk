#!/usr/bin/ruby
f = File.new(ARGV[0], "w")
Dir["_include/lastfm/*"].each do |h| 
	f.write %Q{#include "lastfm/#{File.basename h}"\n}
end