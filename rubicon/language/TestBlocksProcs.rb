$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestBlocksProcs < Rubicon::TestCase

  def testBasicProc
    proc = proc {|i| i}
    assert_equal(2, proc.call(2))
    assert_equal(3, proc.call(3))
    
    proc = proc {|i| i*2}
    assert_equal(4, proc.call(2))
    assert_equal(6, proc.call(3))
  end

  def testProcScoping
    my_proc1 = nil
    my_proc2 = nil
    x = 0
    proc { 
      iii=5	
      my_proc1 = proc{|i|
        iii = i
      }
      my_proc2 = proc {
        x = iii			# nested variables shared by procs
      }
      # scope of nested variables
      assert(defined?(iii))
    }.call
    assert(!defined?(iii))		# out of scope

    my_proc1.call(5)
    my_proc2.call
    assert_equal(5, x)
  end
end

# Run these tests if invoked directly

Rubicon::handleTests(TestBlocksProcs) if $0 == __FILE__
