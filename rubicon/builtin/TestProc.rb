require '../rubicon'


class TestProc < Rubicon::TestCase

  def test_AREF # '[]'
    assert_fail("untested")
  end

  def test_arity
    assert_fail("untested")
  end

  def test_call
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestProc) if $0 == __FILE__
