require "#{File.dirname __FILE__}/platform.rb"

class QMakeNotFound < RuntimeError; end
class QMakeTooOld < RuntimeError; end

def which_qmake
  args = '-v'
  args += ' 2> /dev/null' unless Platform::IMPL == :mswin

  versions = Hash.new
  ['qmake','qmake-qt4'].each do |qmake|
    begin
      /^Using Qt version (\d\.\d\.\d)(-(.+))?/.match( `#{qmake} #{args}` )
    rescue
    end
    versions[qmake] = $1 unless $1.nil?
  end

  raise QMakeNotFound if versions.empty? 

  versions.each do |key, v|
    i = 1
    j = 0
    v.split( '.' ).reverse.each {|n| j += (n.to_i * i); i *= 100}
    versions[key] = j
  end

  versions.sort {|a,b| a[1]<=>b[1]}

  versions.each do |k, v| 
    if v >= 40400
      return k
    end
    raise QMakeTooOld
  end
end

puts which_qmake if __FILE__ == $0