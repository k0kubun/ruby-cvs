require '../rubicon'


class AllBuiltinTests
  
  def AllBuiltinTests.suite
    suite = Rubicon::TestSuite.new
    Dir["Test*.rb"].each { |file|
      require file
      suite.add_test(eval(file.sub(/\.rb$/, '')).suite)
    }
    suite
  end
end

#Rubicon::handleTests(AllBuiltinTests) if $0 == __FILE__

tests = Rubicon::BulkTestRunner.new("Built-ins")

Dir["Test*.rb"].each { |file| tests.addFile(file) }

tests.run
