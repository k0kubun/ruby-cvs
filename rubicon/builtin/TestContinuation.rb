require '../rubicon'


class TestContinuation < Rubicon::TestCase

  def test_call
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestContinuation) if $0 == __FILE__
