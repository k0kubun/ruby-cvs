$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

TimesClass = $rubyVersion >= "1.7" ? Process : Time

class TestStruct__Tms < Rubicon::TestCase

  def sillyCalculation(n)
    a = 0.0; n.times {|i| a += Math.sin(1/(i+1)) }
  end

  def setup
    pid = fork

    name = pid ? "_parent" : "_child"

    Dir.rmdir(name) if File.exists?(name)

    5000.times {
      Dir.mkdir(name)
      Dir.rmdir(name)
    }

    exit unless pid

    Process.wait
  end

  def cstime
    assert(TimesClass.times.cstime > 0.0)
  end

  def cutime
    assert(TimesClass.times.cutime > 0.0)
  end

  def stime
    assert(TimesClass.times.stime > 0.0)
  end

  def utime
    assert(TimesClass.times.utime > 0.0)
  end

  def s_aref
    assert(TimesClass.times['utime'] != 0)
    assert(TimesClass.times['stime'] != 0)
    assert(TimesClass.times['cutime'] != 0)
    assert(TimesClass.times['cstime'] != 0)
  end

  def s_members
    assert_equals(4, TimesClass.times.members.length)
  end
  
  def test_all
    cstime()
    cutime()
    stime()
    utime()
    s_aref()
    s_members()
  end

end

Rubicon::handleTests(TestStruct__Tms) if $0 == __FILE__
