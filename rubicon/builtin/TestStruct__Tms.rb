require '../rubicon'


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
    assert(Time.times.cstime > 0.0)
  end

  def cutime
    assert(Time.times.cutime > 0.0)
  end

  def stime
    assert(Time.times.stime > 0.0)
  end

  def utime
    assert(Time.times.utime > 0.0)
  end

  def s_aref
    assert(Time.times['utime'] != 0)
    assert(Time.times['stime'] != 0)
    assert(Time.times['cutime'] != 0)
    assert(Time.times['cstime'] != 0)
  end

  def s_members
    assert_equals(4, Time.times.members.length)
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
