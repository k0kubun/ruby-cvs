module Rubicon

  require 'runit/testcase'
  require 'runit/cui/testrunner'

  # -------------------------------------------------------

  class TestResult < RUNIT::TestResult
  end

  # -------------------------------------------------------

  class TestRunner < RUNIT::CUI::TestRunner
    def create_result
      TestResult.new
    end
  end

  # -------------------------------------------------------

  class TestSuite < RUNIT::TestSuite
  end


  # -------------------------------------------------------

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
      if true
        $stderr.print "S"
      else
        $stderr.puts "\nSkipping: #{from} - #{info}"
      end
    end

    #
    # Handle broken exception handling
    #
#     def assert_exception(ex, &code)
#       begin
#         super
#       rescue ex
#         if $!.instance_of? ex
#           $stderr.puts "\nThis RubyUnit does not trap #{ex}. This error can\n" +
#           "safely be ignored"
#         else
#           raise
#         end
#       end
#     end

    #
    # Check a float for approximate equality
    #
    def assert_flequal(exp, actual, msg='')
      if exp == 0.0
        error = 1e-7
      else
        error = exp.abs/1e7
      end
      
      assert((exp - actual).abs < error, 
             "#{msg} Expected #{'%f' % exp} got #{'%f' % actual}")
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
      pid = fork 
      if pid.nil?
          block.call
          exit 0
      end
      Process.waitpid(pid, 0)
      return ($? >> 8) & 0xff
    end

    def setup
      super
    end

    def teardown
      begin
        loop { Process.wait; puts "\n\nCHILD REAPED\n\n" }
      rescue Errno::ECHILD
      end
      super
    end
    #
    # Setup some files in a test directory.
    #
    def setupTestDir
      @start = Dir.getwd
      teardownTestDir
      Dir.mkdir("_test")
      if $? != 0 && false
        $stderr.puts "Cannot run a file or directory test: " + 
          "will destroy existing directory _test"
        exit(99)
      end
      Dir.chdir("_test")
      File.open("_file1", "w", 0644) {}
      File.open("_file2", "w", 0644) {}
      Dir.chdir("..")
      @files = %w(. .. _file1 _file2)
    end
    
    def teardownTestDir
      Dir.chdir(@start)
      if (File.exists?("_test"))
        Dir.chdir("_test")
        Dir.foreach(".") { |f| File.delete(f) unless f[0] == ?. }
        Dir.chdir("..")
        Dir.rmdir("_test")
      end
    end

  end

    
  #
  # Common code to run the tests in a class
  #
  def handleTests(testClass)
    testrunner = TestRunner.new
#    TestRunner.quiet_mode = true
    if ARGV.size == 0
      suite = testClass.suite
    else
      suite = RUNIT::TestSuite.new
      ARGV.each do |testmethod|
        suite.add_test(testClass.new(testmethod))
      end
    end
    results = testrunner.run(suite)
  end
  module_function :handleTests


  # Accumulate a running set of results, and report them at the end
  
  class ResultGatherer

    LINE_LENGTH = 72
    LINE = '=' * LINE_LENGTH
    Line = ' ' * 12 + '-' * (LINE_LENGTH - 12)

    def initialize(name)
      @name    = name
      @results = {}
    end

    def add(klass, resultSet)
      @results[klass.name] = resultSet
    end
    
    def report
      puts
      puts LINE
      title = "Test Results".center(LINE_LENGTH)
      title[0, @name.length] = @name
      puts title
      puts LINE
      puts "            Name   OK?   Tests  Asserts      Failures   Errors"
      puts Line

      total_classes = 0
      total_tests   = 0
      total_asserts = 0
      total_fails   = 0
      total_errors  = 0
      total_bad     = 0

      format = "%16s   %4s   %4d  %7d  %9s  %7s\n"

      names = @results.keys.sort
      for name in names
        res    = @results[name]
        fails  = res.failure_size.nonzero? || ''
        errors = res.error_size.nonzero? || ''

        total_classes += 1
        total_tests   += res.run_tests
        total_asserts += res.run_asserts
        total_fails   += res.failure_size
        total_errors  += res.error_size
        total_bad     += 1 unless res.succeed?

        printf format,
          name.sub(/^Test/, ''),
          res.succeed? ? "    " : "FAIL",
          res.run_tests, res.run_asserts, 
          fails.to_s, errors
      end

      puts LINE
      if total_classes > 1
        printf format, 
          sprintf("All %d files", total_classes),
          total_bad > 0 ? "FAIL" : "    ",
          total_tests, total_asserts,
          total_fails, total_errors
        puts LINE
      end

      if total_fails > 0
        puts
        puts "Failure Report".center(LINE_LENGTH)
        puts LINE
        left = total_fails

        for name in names
          res = @results[name]
          if res.failure_size > 0
            puts
            puts name + ":"
            puts "-" * name.length.succ

            res.failures.each do |f|
              f.at.each do |at|
                break if at =~ /rubicon/
                print "    ", at, "\n"
              end
              err = f.err.to_s

              if err =~ /expected:(.*)but was:(.*)/m
                exp = $1.dump
                was = $2.dump
                print "    ....Expected: #{exp}\n"
                print "    ....But was:  #{was}\n"
              else
                print "    ....#{err}\n"
              end
            end

            left -= res.failure_size
            puts
            puts Line if left > 0
          end
        end
        puts LINE
      end

      if total_errors > 0
        puts
        puts "Error Report".center(LINE_LENGTH)
        puts LINE
        left = total_errors

        for name in names
          res = @results[name]
          if res.error_size > 0
            puts
            puts name + ":"
            puts "-" * name.length.succ

            res.errors.each do |f|
              f.at.each do |at|
                break if at =~ /rubicon/
                print "    ", at, "\n"
              end
              err = f.err.to_s
              print "    ....#{err}\n"
            end

            left -= res.error_size
            puts
            puts Line if left > 0
          end
        end
        puts LINE
      end

    end
  end


  # Run a set of tests in a file. This would be a TestSuite, but we
  # want to run each file separately, and to summarize the results
  # differently

  class BulkTestRunner

    def initialize(groupName)
      @groupName = groupName
      @files     = []
      @results   = ResultGatherer.new(groupName)
    end

    def addFile(fileName)
      @files << fileName
    end

    def run
      @files.each do |file|
        require file
        className = File.basename(file)
        className.sub!(/\.rb$/, '')
        klass = eval className
        runner = TestRunner.new
        TestRunner.quiet_mode = true
        $stderr.print "\n", className, ": "

        @results.add(klass, runner.run(klass.suite))
      end

      @results.report
    end

  end
end



