require '../rubicon'


class TestSymbol < Rubicon::TestCase

  def test_id2name
    assert_fail("untested")
  end

  def test_inspect
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

Rubicon::handleTests(TestSymbol) if $0 == __FILE__
