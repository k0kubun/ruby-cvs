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

    def truth_table(method, *result)
      for a in [ false, true ]
        res = result.shift
        assert_equal(method.call(a), res)
        assert_equal(method.call(a ? self : nil), res)
      end
    end

    # 
    # Report we're skipping a test
    #
    def skipping(info, from=nil)
      unless from
        caller[0] =~ /`(.*)'/ #`
        from = $1
      end
      $stderr.puts "\nSkipping: #{from} - #{info}"
    end

    #
    # Skip a test if not super user
    #
    def super_user
      caller[0] =~ /`(.*)'/ #`
      skipping("not super user", $1)
    end

    #
    # Issue a system and abort on error
    #
    def sys(cmd)
      assert(system(cmd), cmd)
      assert_equal(0, $?, "cmd: #{$?}")
    end

    #
    # Check two arrays for set equality
    #
    def assert_set_equal(expected, actual)
      assert_equal([], (expected - actual) | (actual - expected),
                   "Expected: #{expected.inspect}, Actual: #{actual.inspect}")
    end

    #
    # Run a block in a sub process and return exit status
    #
    def runChild(&block)
      pid=fork 
      if pid.nil?
          block.call
          exit 0
      end
      Process.waitpid(pid,0)
      return ($? >> 8) & 0xff
    end
    

    def setupTestDir
      @start = Dir.getwd
 
      system("mkdir _test")
      if $? != 0 && false
        $stderr.puts "Cannot run a file or directory test: " + 
          "will destroy existing directory _test"
        exit(99)
      end
      sys("touch _test/_file1")
      sys("touch _test/_file2")
      @files = %w(. .. _file1 _file2)
    end
    
    def teardownTestDir
      Dir.chdir(@start)
      system("rm -f _test/*")
      system("rmdir _test 2>/dev/null")
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
