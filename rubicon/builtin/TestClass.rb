require '../rubicon'


class TestClass < Rubicon::TestCase

  def test_new
    assert_fail("untested")
  end

  def test_superclass
    assert_fail("untested")
  end

  def test_s_constants
    assert_fail("untested")
  end

  def test_s_inherited
    assert_fail("untested")
  end

  def test_s_nesting
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestClass) if $0 == __FILE__
