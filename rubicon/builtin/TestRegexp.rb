require '../rubicon'


class TestRegexp < Rubicon::TestCase

  def test_EQUAL # '=='
    assert_fail("untested")
  end

  def test_MATCH # '=~'
    assert_fail("untested")
  end

  def test_REV # '~'
    assert_fail("untested")
  end

  def test_VERY_EQUAL # '==='
    assert_fail("untested")
  end

  def test_casefold?
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_kcode
    assert_fail("untested")
  end

  def test_last_match
    assert_fail("untested")
  end

  def test_match
    assert_fail("untested")
  end

  def test_source
    assert_fail("untested")
  end

  def test_s_compile
    assert_fail("untested")
  end

  def test_s_escape
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

  def test_s_quote
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestRegexp) if $0 == __FILE__
