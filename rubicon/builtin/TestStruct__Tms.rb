require '../rubicon'


class TestStruct__Tms < Rubicon::TestCase

  def sillyCalculation(n)
    a = 0.0; n.times {|i| a += Math.sin(1/(i+1)) }
  end

  def burntime
    # burn up user CPU
    sillyCalculation(20000)
    # and system CPU
    200.times {
      fork { sillyCalculation(100) }
      Process.wait
    }
  end

  def setup
    # Make a child process. Run the process in both the child
    # and the parent to get decent times. RUn in parallel so
    # those with MP boxes get home slightly faster
    burntime
    puts Time.times.to_a
  end

  def test_cstime
    assert(Time.times.cstime != 0)
  end

  def test_cutime
    assert(Time.times.cutime != 0)
  end

  def test_stime
    assert(Time.times.stime != 0)
  end

  def test_utime
    assert(Time.times.utime != 0)
  end

  def test_s_aref
    assert(Time.times['utime'] != 0)
    assert(Time.times['stime'] != 0)
    assert(Time.times['cutime'] != 0)
    assert(Time.times['cstime'] != 0)
  end

  def test_s_members
    assert_equals(4,Time.times.members.length)
  end

end

Rubicon::handleTests(TestStruct__Tms) if $0 == __FILE__
