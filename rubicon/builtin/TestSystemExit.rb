require '../rubicon'


class TestSystemExit < Rubicon::TestCase

  def test_s_exception
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestSystemExit) if $0 == __FILE__
