require '../rubicon'


class TestNumeric < Rubicon::TestCase

  def test_UPLUS
    assert_fail("untested")
  end

  def test_UMINUS
    assert_fail("untested")
  end

  def test_VERY_EQUAL # '==='
    assert_fail("untested")
  end

  def test_abs
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_coerce
    assert_fail("untested")
  end

  def test_divmod
    assert_fail("untested")
  end

  def test_eql?
    assert_fail("untested")
  end

  def test_integer?
    assert_fail("untested")
  end

  def test_nonzero?
    assert_fail("untested")
  end

  def test_zero?
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestNumeric) if $0 == __FILE__
