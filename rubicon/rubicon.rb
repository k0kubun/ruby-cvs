module Rubicon

  require 'runit/testcase'
  require 'runit/cui/testrunner'

  class TestSuite < RUNIT::TestSuite
  end

  class TestCase < RUNIT::TestCase

    # Local routine to check that a set of bits, and only a set of bits,
    # is set!
    def checkBits(bits, num)
      0.upto(90)  { |n|
        expected = bits.include?(n) ? 1 : 0
        assert_equal(expected, num[n], "bit %d" % n)
      }
    end
  end

  def handleTests(testClass)
    testrunner = RUNIT::CUI::TestRunner.new
    RUNIT::CUI::TestRunner.quiet_mode = true
    if ARGV.size == 0
      suite = testClass.suite
    else
      suite = RUNIT::TestSuite.new
      ARGV.each do |testmethod|
        suite.add_test(testClass.new(testmethod))
      end
    end
    testrunner.run(suite)
  end
  module_function :handleTests
end
