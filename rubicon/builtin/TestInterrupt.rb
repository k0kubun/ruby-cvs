require '../rubicon'


class TestInterrupt < Rubicon::TestCase

  def test_s_exception
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestInterrupt) if $0 == __FILE__
