require '../rubicon'


class TestStruct < Rubicon::TestCase

  def test_AREF # '[]'
    assert_fail("untested")
  end

  def test_ASET # '[]='
    assert_fail("untested")
  end

  def test_EQUAL # '=='
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_each
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_length
    assert_fail("untested")
  end

  def test_members
    assert_fail("untested")
  end

  def test_size
    assert_fail("untested")
  end

  def test_to_a
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_values
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestStruct) if $0 == __FILE__
