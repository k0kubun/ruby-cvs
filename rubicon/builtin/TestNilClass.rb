require '../rubicon'


class TestNilClass < Rubicon::TestCase

  def test_AND # '&'
    assert_fail("untested")
  end

  def test_OR # '|'
    assert_fail("untested")
  end

  def test_XOR # '^'
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_nil?
    assert_fail("untested")
  end

  def test_to_a
    assert_fail("untested")
  end

  def test_to_i
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_type
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestNilClass) if $0 == __FILE__
