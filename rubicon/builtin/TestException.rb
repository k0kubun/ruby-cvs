require '../rubicon'


class TestException < Rubicon::TestCase

  def test_backtrace
    assert_fail("untested")
  end

  def test_exception
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_message
    assert_fail("untested")
  end

  def test_set_backtrace
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_to_str
    assert_fail("untested")
  end

  def test_s_exception
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestException) if $0 == __FILE__
