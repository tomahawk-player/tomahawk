#
# platform.rb: naive platform detection for Ruby
# author: Matt Mower <self@mattmower.com>
#

# == Platform
#
# Platform is a simple module which parses the Ruby constant
# RUBY_PLATFORM and works out the OS, it's implementation,
# and the architecture it's running on.
#
# The motivation for writing this was coming across a case where
#
# +if RUBY_PLATFORM =~ /win/+
#
# didn't behave as expected (i.e. on powerpc-darwin-8.1.0)
#
# It is hoped that providing a library for parsing the platform
# means that we can cover all the cases and have something which
# works reliably 99% of the time.
#
# Please report any anomalies or new combinations to the author(s).
#
# == Use
#
# require "platform"
#
# defines
#
# Platform::OS (:unix,:win32,:vms,:os2)
# Platform::IMPL (:macosx,:linux,:mswin)
# Platform::ARCH (:powerpc,:x86,:alpha)
#
# if an unknown configuration is encountered any (or all) of
# these constant may have the value :unknown.
#
# To display the combination for your setup run
#
# ruby platform.rb
#
module Platform
   
   if RUBY_PLATFORM =~ /darwin/i
      OS = :unix
      IMPL = :macosx
   elsif RUBY_PLATFORM =~ /linux/i
      OS = :unix
      IMPL = :linux
   elsif RUBY_PLATFORM =~ /freebsd/i
      OS = :unix
      IMPL = :freebsd
   elsif RUBY_PLATFORM =~ /netbsd/i
      OS = :unix
      IMPL = :netbsd
   elsif RUBY_PLATFORM =~ /mswin/i
      OS = :win32
      IMPL = :mswin
   elsif RUBY_PLATFORM =~ /cygwin/i
      OS = :win32
      IMPL = :mswin
   elsif RUBY_PLATFORM =~ /mingw/i
      OS = :win32
      IMPL = :mingw
   elsif RUBY_PLATFORM =~ /bccwin/i
      OS = :win32
      IMPL = :bccwin
   elsif RUBY_PLATFORM =~ /wince/i
      OS = :win32
      IMPL = :wince
   elsif RUBY_PLATFORM =~ /vms/i
      OS = :vms
      IMPL = :vms
   elsif RUBY_PLATFORM =~ /os2/i
      OS = :os2
      IMPL = :os2 # maybe there is some better choice here?
   else
      OS = :unknown
      IMPL = :unknown
   end
   
   # whither AIX, SOLARIS, and the other unixen?
   
   if RUBY_PLATFORM =~ /(i\d86)/i
      ARCH = :x86
   elsif RUBY_PLATFORM =~ /ia64/i
      ARCH = :ia64
   elsif RUBY_PLATFORM =~ /powerpc/i
      ARCH = :powerpc
   elsif RUBY_PLATFORM =~ /alpha/i
      ARCH = :alpha
   else
      ARCH = :unknown
   end
   
   # What about AMD, Turion, Motorola, etc..?
   
end

if __FILE__ == $0
   puts "Platform OS=#{Platform::OS}, IMPL=#{Platform::IMPL}, ARCH=#{Platform::ARCH}"
end
