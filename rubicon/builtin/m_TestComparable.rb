require '../rubicon'


class TestComparable < Rubicon::TestCase

  def test_EQUAL # '=='
    assert_fail("untested")
  end

  def test_GE # '>='
    assert_fail("untested")
  end

  def test_GT # '>'
    assert_fail("untested")
  end

  def test_LE # '<='
    assert_fail("untested")
  end

  def test_LT # '<'
    assert_fail("untested")
  end

  def test_between?
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestComparable) if $0 == __FILE__
