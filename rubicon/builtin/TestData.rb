require '../rubicon'


class TestData < Rubicon::TestCase

end

Rubicon::handleTests(TestData) if $0 == __FILE__
