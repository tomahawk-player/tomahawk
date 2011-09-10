cwd = File.dirname( __FILE__ )

def h(s, n)
    puts '==> '+s
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
