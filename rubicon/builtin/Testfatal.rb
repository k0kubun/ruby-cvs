require '../rubicon'


class Testfatal < Rubicon::TestCase

end

Rubicon::handleTests(Testfatal) if $0 == __FILE__
