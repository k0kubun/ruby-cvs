require '../rubicon'


class TestBinding < Rubicon::TestCase


end

Rubicon::handleTests(TestBinding) if $0 == __FILE__
