require "tempfile"
require "rbconfig.rb"

#
# Simple wrapper for RubyUnit, primarily designed to capture
# statsistics and report them at the end.
#

RUBICON_VERSION = "V0.3"

# -------------------------------------------------------------
#
# Operating system classification. We use classes for this, as 
# we get lots of flexibility with comparisons.
#
# Use with
#
#   Unix.or_varient do ... end        # operating system is some Unix variant
#
#   Linux.only do .... end            # operating system is Linux
#
#   MsWin32.dont do .... end          # don't run under MsWin32

class OS
  def OS.or_variant
    yield if $os <= self
  end

  def OS.only
    yield if $os == self
  end

  def OS.dont
    yield unless $os <= self
  end
end

class Unix    < OS;      end
class Linux   < Unix;    end
class BSD     < Unix;    end
class FreeBSD < BSD;     end

class Windows < OS;      end
class Cygwin  < Windows; end
class MsWin32 < Windows; end

$os = case RUBY_PLATFORM
      when /linux/   then  Linux
      when /bsd/     then BSD
      when /cygwin/  then Cygwin
      when /mswin32/ then MsWin32
      else OS
      end

#
# Find the name of the interpreter.
# 

$interpreter = File.join(Config::CONFIG["bindir"], 
			 Config::CONFIG["RUBY_INSTALL_NAME"])

Windows.or_variant { $interpreter.tr! '/', '\\' }


######################################################################
#
# This is tacky, but... We need to be able tofind the executable
# files in the util subdirectory. However, we can be initiated by
# running a file in either the tpo-level rubicon directory or in
# one of its test subdirectories (such as language). We therefore
# need tpo hunt around for the util directory

run_dir = File.dirname($0)

for relative_path in [ ".", ".." ]
  util = File.join(run_dir, relative_path, "util")

  if File.exist?(util) and File.directory?(util)
    UTIL = util
    break
  end
end

raise "Cannot find 'util' directory" unless defined?(UTIL)

CHECKSTAT = File.join(UTIL, "checkstat")
TEST_TOUCH = File.join(UTIL, "test_touch")

MsWin32.only do
  CHECKSTAT << ".exe"
  TEST_TOUCH << ".exe"
end

for file in [CHECKSTAT, TEST_TOUCH]
  raise "Cannot find #{file}" unless File.exist?(file)
end

#
# Classification routines. We use these so that the code can
# test for operating systems, ruby versions, and other features
# without being platform specific
#

# -------------------------------------------------------
# Class to manipulate Ruby version numbers. We use this to 
# insulate ourselves from changes in version number format.
# Independent of the internal representation, we always allow 
# comparison against a string.
#
# Use in the code with stuff like:
#
#    if $rubyVersion <= "1.6.2" 
#       asert(...)
#    end
#

class VersionNumber
  include Comparable
  
  def initialize(version)
    @version = version
  end
  
  def <=>(other)
    @version <=> other
  end
end

$rubyVersion = VersionNumber.new(VERSION)



#
# This is the main Rubicon module, implemented as a module to
# protect the namespace a tad
#

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

    def assert_kindof_exception(exception, message="")
      setup_assert
      block = proc
      exception_raised = true
      err = ""
      ret = nil
      begin
	block.call
	exception_raised = false
	err = 'NO EXCEPTION RAISED'
      rescue Exception
	if $!.kind_of?(exception)
	  exception_raised = true
	  ret = $!
	else
	  raise $!.type, $!.message, $!.backtrace
	end
      end
      if !exception_raised
      	msg = edit_message(message)
        msg.concat "expected:<"
	msg.concat to_str(exception)
	msg.concat "> but was:<"
	msg.concat to_str(err)
	msg.concat ">"
	raise_assertion_error(msg, 2)
      end
      ret
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
      if $os == MsWin32
	assert(system(cmd), "command failed: #{cmd}")
      else
	assert(system(cmd), cmd + ": #{$? >> 8}")
	assert_equal(0, $?, "cmd: #{$?}")
      end
    end

    #
    # Use our 'test_touch' utility to touch a file
    #
    def touch(arg)
#      puts("#{TEST_TOUCH} #{arg}")
      sys("#{TEST_TOUCH} #{arg}")
    end

    #
    # And out checkstat utility to get the status
    #
    def checkstat(arg)
#      puts("#{CHECKSTAT} #{arg}")
      `#{CHECKSTAT} #{arg}`
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
      if $os != MsWin32
	begin
	  loop { Process.wait; puts "\n\nCHILD REAPED\n\n" }
	rescue Errno::ECHILD
	end
      end
      super
    end
    #
    # Setup some files in a test directory.
    #
    def setupTestDir
      @start = Dir.getwd
      teardownTestDir
      begin
	Dir.mkdir("_test")
      rescue
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
    
    def deldir(name)
      File.chmod(0755, name)
      Dir.chdir(name)
      Dir.foreach(".") do |f|
        next if f == '.' || f == '..'
        if File.lstat(f).directory?
          deldir(f) 
        else
          File.chmod(0644, f) rescue true
          File.delete(f)
        end 
      end
      Dir.chdir("..")
      Dir.rmdir(name)
    end

    def teardownTestDir
      Dir.chdir(@start)
      deldir("_test") if (File.exists?("_test"))
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


  # Record a particule failure, which is a location
  # and an error message. We simply ape the runit
  # TestFailure class.

  class Failure
    attr_accessor :at
    attr_accessor :err
    
    def Failure.from_real_failures(f)
      f.collect do |a_failure|
        my_f = Failure.new
        my_f.at = a_failure.at
        my_f.err = a_failure.err
        my_f
      end
    end
  end

  # Objects of this class get generated from the TestResult
  # passed back by RUnit. We don't use it's class for two reasons:
  # 1. We decouple better this way
  # 2. We can't serialize the RUnit class, as it contains IO objects
  #

  
  class Results
    attr_reader :failure_size
    attr_reader :error_size
    attr_reader :run_tests
    attr_reader :run_asserts
    attr_reader :failures
    attr_reader :errors

    def initialize_from(test_result)
      @failure_size = test_result.failure_size
      @error_size   = test_result.error_size
      @run_tests    = test_result.run_tests
      @run_asserts  = test_result.run_asserts
      @succeed      = test_result.succeed?
      @failures     = Failure.from_real_failures(test_result.failures)
      @errors       = Failure.from_real_failures(test_result.errors)
      self
    end

    def succeed?
      @succeed
    end
  end

  # And here is where we gather the results of all the tests. This is
  # also the object exported to XML

  class ResultGatherer

    attr_reader   :results
    attr_accessor :name
    attr_reader   :config
    attr_reader   :date
    attr_reader   :rubicon_version

    def initialize(name = '')
      @name    = ''
      @results = {}
      @config  = Config::CONFIG
      @date    = Time.now
      @rubicon_version = RUBICON_VERSION
    end
    
    def add(klass, result_set)
      @results[klass.name] = Results.new.initialize_from(result_set)
    end
    
  end

  # Run a set of tests in a file. This would be a TestSuite, but we
  # want to run each file separately, and to summarize the results
  # differently

  class BulkTestRunner

    def initialize(args, group_name)
      @groupName = group_name
      @files     = []
      @results   = ResultGatherer.new
      @results.name   = group_name
      @op_class_file  = "ascii"

      # Look for a -op <class> argument, which controls
      # where our output goes

      if args.size > 1 and args[0] == "-op"
        args.shift
        @op_class_file = args.shift
      end

      @op_class_file = "result_" + @op_class_file
      require @op_class_file
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

      reporter = ResultDisplay.new(@results)
      reporter.reportOn $stdout
    end

  end
end



