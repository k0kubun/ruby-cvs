require '../rubicon'


class TestMarshal < Rubicon::TestCase

  def test_s_dump
    assert_fail("untested")
  end

  def test_s_load
    assert_fail("untested")
  end

  def test_s_restore
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestMarshal) if $0 == __FILE__
