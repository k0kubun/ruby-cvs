require '../rubicon'


class TestMethod < Rubicon::TestCase

  def test_AREF # '[]'
    assert_fail("untested")
  end

  def test_arity
    assert_fail("untested")
  end

  def test_call
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_to_proc
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestMethod) if $0 == __FILE__
