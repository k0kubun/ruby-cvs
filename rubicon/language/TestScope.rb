$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestScope < Rubicon::TestCase

end

# Run these tests if invoked directly

Rubicon::handleTests(TestScope) if $0 == __FILE__
