$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestObject < Rubicon::TestCase

end

Rubicon::handleTests(TestObject) if $0 == __FILE__
