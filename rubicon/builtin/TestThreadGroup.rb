$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestThreadGroup < Rubicon::TestCase

  def test_00sanity
    tg = ThreadGroup::Default
    assert_equal( Thread.current, tg.list[0])
  end

  def test_add
    tg = ThreadGroup.new
    tg.add(Thread.current)
    assert_equal(1, tg.list.length)
    assert_equal(0, ThreadGroup::Default.list.length)
  end

  def test_list
    tg = ThreadGroup.new
    10.times {
      t = Thread.new { sleep 60 }
      tg.add(t)
    }
    assert_equals(tg.list.length,10)
  end

  def test_s_new
    tg = ThreadGroup.new
    assert_equal(0, tg.list.length)
  end

end

Rubicon::handleTests(TestThreadGroup) if $0 == __FILE__
