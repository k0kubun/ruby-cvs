require '../rubicon'


class TestSignalException < Rubicon::TestCase

  def test_s_exception
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestSignalException) if $0 == __FILE__
