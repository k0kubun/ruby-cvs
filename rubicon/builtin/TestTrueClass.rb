require '../rubicon'


class TestTrueClass < Rubicon::TestCase

  def test_AND # '&'
    assert_fail("untested")
  end

  def test_OR # '|'
    assert_fail("untested")
  end

  def test_XOR # '^'
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_type
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestTrueClass) if $0 == __FILE__
