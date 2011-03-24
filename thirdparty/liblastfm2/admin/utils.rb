cwd = File.dirname( __FILE__ )
require "#{cwd}/platform.rb"

def h(s, n)
  case Platform::IMPL
  when :mswin
    puts '==> '+s
  else
    puts "\033[0;#{n}m==>\033[0;0;1m #{s} \033[0;0m"
  end
end

def h1 s
    h(s, 34)
end

def h2 s
    h(s, 33)
    yield
end

def qmake_env(env, qenv)
  env=Array.new(1,env) if env.instance_of? String
  values=Array.new
  env.each { |x| values << ENV[x] if ENV[x] }
  if values.size > 0
    "#{qenv} = #{values.join(' ')}\n"
  else
    nil
  end
end

class PkgConfigNotFound < RuntimeError; end
class PkgNotFound < RuntimeError; end

def pkgconfig pkg, prettyname
  system "pkg-config --exists '#{pkg}'"
  raise PkgConfigNotFound if $? == 127
  raise PkgNotFound.new(prettyname) if $? != 0
end