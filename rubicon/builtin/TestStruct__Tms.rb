require '../rubicon'


class TestStruct__Tms < Rubicon::TestCase

  def test_00init
    # Make a child process.
    `ruby -e 'a = 0; 10000.times {|i| a += Math.sin(1/(i+1)) }'`
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
