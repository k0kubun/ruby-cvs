require '../rubicon'


class TestBinding < Rubicon::TestCase

  def test_clone
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestBinding) if $0 == __FILE__
