require '../rubicon'


class TestGC < Rubicon::TestCase

  def test_garbage_collect
    assert_fail("untested")
  end

  def test_s_disable
    assert_fail("untested")
  end

  def test_s_enable
    assert_fail("untested")
  end

  def test_s_start
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestGC) if $0 == __FILE__
