$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestThread < Rubicon::TestCase

  def setup
    puts "******************" if Thread.critical
  end

  def thread_control
    @ready = false
    yield
    @ready = false
  end

  def _signal
    @ready = true
  end

  def _wait
    sleep .1 while !@ready
    @ready = false
  end

  class SubThread < Thread
    def initialize
      @wasCalled = true
      super
    end
    def initCalled?
      @wasCalled
    end
  end
  
  def teardown
    Thread.list.each do |t|
      if t != Thread.main
        t.kill
      end
    end
  end

  def test_AREF # '[]'
    t = Thread.current
    t2 = Thread.new { sleep 60 }

    t[:test] = "alpha"
    t2[:test] = "gamma"
    assert_equals(t[:test], "alpha")
    assert_equals(t2[:test], "gamma")
    t["test"] = "bravo"
    t2["test"] = "delta"
    assert_equals(t["test"], "bravo")
    assert_equals(t2["test"], "delta")
    assert(t[:none].nil?)
    assert(t["none"].nil?)
    assert(t2[:none].nil?)
    assert(t2["none"].nil?)
  end

  def test_ASET # '[]='
    t = Thread.current
    t2 = Thread.new { sleep 60 }

    t[:test] = "alpha"
    t2[:test] = "gamma"
    assert_equals(t[:test], "alpha")
    assert_equals(t2[:test], "gamma")
    t["test"] = "bravo"
    t2["test"] = "delta"
    assert_equals(t["test"], "bravo")
    assert_equals(t2["test"], "delta")
    assert(t[:none].nil?)
    assert(t["none"].nil?)
    assert(t2[:none].nil?)
    assert(t2["none"].nil?)
  end

  def test_abort_on_exception
    # Test default
    assert_equal(false, Thread.current.abort_on_exception)
    Thread.current.abort_on_exception = true
    assert_equal(true, Thread.current.abort_on_exception)
    Thread.current.abort_on_exception = false
    assert_equal(false, Thread.current.abort_on_exception)
  end

  class MyException < Exception; end

  def test_abort_on_exception=()
    skipping("Can't find a test for all platforms")
    return
    save_stderr = nil
    begin
      begin
        t = Thread.new do
          raise MyException, "boom"
        end
        Thread.pass
        assert(true)
      rescue MyException
        assert_fail("Thread exception propogated to main thread")
      end
      
      begin
        Thread.new do
          Thread.current.abort_on_exception = true
          save_stderr = $stderr
          $stderr = nil
          raise MyException, "boom"
        end
        Thread.pass
        assert_fail("Exception should have interrupted main thread")
      rescue TestThread::MyException
        assert(true)
      ensure
        $stderr = save_stderr
      end
    rescue Exception
      assert_fail($!.to_s)
    end
  end

  def test_alive?
    t1 = t2 = nil
    thread_control do
      t1 = Thread.new { _signal; Thread.stop }
      _wait
    end
    thread_control do
      t2 = Thread.new { _signal; sleep 60 }
      _wait
    end
    t3 = Thread.new {}
    t3.join
    assert_equals(true,Thread.current.alive?)
    assert_equals(true,t1.alive?)
    assert_equals(true,t2.alive?)
    assert_equals(false,t3.alive?)
  end

  def test_exit
    t = Thread.new { Thread.current.exit }
    t.join
    assert_equals(t,t.exit)
    assert_equals(false,t.alive?)
  end

  def test_join
    sum = 0
    t = Thread.new do
      5.times { sum += 1; sleep .1 }
    end
    assert(sum != 5)
    t.join
    assert_equal(5, sum)

    sum = 0
    t = Thread.new do
      5.times { sum += 1; sleep .1 }
    end
    t.join
    assert_equal(5, sum)

    # if you join a thread, it's exceptions become ours
    t = Thread.new do
      Thread.pass
      raise "boom"
    end

    begin
      t.join
    rescue Exception => e
      assert_equals("boom", e.message)
    end
  end

  def test_key?
    t = Thread.current
    t2 = Thread.new { sleep 60 }

    t[:test] = "alpha"
    t2[:test] = "gamma"
    assert_equals(true,t.key?(:test))
    assert_equals(true,t2.key?(:test))
    assert_equals(false,t.key?(:none))
    assert_equals(false,t2.key?(:none))
  end

  def test_kill
    t = Thread.new { Thread.current.kill }
    t.join
    assert_equals(t, t.kill)
    assert_equals(false, t.alive?)
  end

  def test_priority
    assert_equals(0, Thread.current.priority)
  end

  def test_priority=()
    Cygwin.only do
      assert_fail("Thread priorities seem broken under Cygwin")
      return
    end

    c1 = 0
    c2 = 0
    my_priority = Thread.current.priority
    begin
      Thread.current.priority = 10
      a = Thread.new { Thread.stop; loop { c1 += 1 }}
      b = Thread.new { Thread.stop; loop { c2 += 1 }}
      a.priority = my_priority - 2
      b.priority = my_priority - 1
      1 until a.stop? and b.stop?
      a.wakeup
      b.wakeup
      sleep 1
      Thread.critical = true
      begin
	assert (c2 > c1)
	c1 = 0
	c2 = 0
	a.priority = my_priority - 1
	b.priority = my_priority - 2
	Thread.critical = false
	sleep 1 
	Thread.critical = true
	assert (c1 > c2)
	a.kill
	b.kill
      ensure
	Thread.critical = false
      end
    ensure
      Thread.current.priority = my_priority
    end
  end

  def test_raise
    madeit = false
    t = nil

    thread_control do
      t = Thread.new do
	_signal
	sleep 5
	madeit = true 
      end
      _wait
    end
    t.raise "Gotcha"
    assert(!t.alive?)
    assert_equals(false,madeit)
  end

  def test_run
    wokeup = false
    t1 = nil
    thread_control do
      t1 = Thread.new { _signal; Thread.stop; wokeup = true ; _signal}
      _wait
      assert_equals(false, wokeup)
      t1.run
      _wait
      assert_equals(true, wokeup)
    end

    wokeup = false
    thread_control do
      t1 = Thread.new { _signal; Thread.stop; _signal; wokeup = true }
      _wait

      assert_equals(false, wokeup)
      Thread.critical = true
      t1.run
      assert_equals(false, wokeup)
      Thread.critical = false
      t1.run
      _wait
      t1.join
      assert_equals(true, wokeup)
    end
  end

  def test_safe_level
    t = Thread.new do
      assert_equals(0, Thread.current.safe_level)
      $SAFE=1
      assert_equals(1, Thread.current.safe_level)
      $SAFE=2
      assert_equals(2, Thread.current.safe_level)
      $SAFE=3
      assert_equals(3, Thread.current.safe_level)
      $SAFE=4
      assert_equals(4, Thread.current.safe_level)
      Thread.pass
    end
    assert_equals(0, Thread.current.safe_level)
    assert_equals(4, t.safe_level)
    t.kill
  end

  def test_status
    a = b = c = nil

    thread_control do
      a = Thread.new { _signal; raise "dead" }
      _wait
    end
    
    thread_control do
      b = Thread.new { _signal; Thread.stop }
      _wait
    end

    thread_control do
      c = Thread.new { _signal;  }
      _wait
    end

    assert_equals("run",   Thread.current.status)
    assert_equals(nil,     a.status)
    assert_equals("sleep", b.status)
    assert_equals(false,   c.status)
  end

  def test_stop?
    a = nil
    thread_control do
      a = Thread.new { _signal; Thread.stop }
      _wait
    end
    assert_equals(true, a.stop?)
    assert_equals(false, Thread.current.stop?)
  end

  def test_value
    t=[]
    10.times { |i|
      t[i] = Thread.new { i }
    }
    result = 0
    10.times { |i|
      result += t[i].value
    }
    assert_equals(45, result)
  end

  def test_wakeup
    madeit = false
    t = Thread.new { Thread.stop; madeit = true }
    assert_equals(false, madeit)
    Thread.pass while t.status != "sleep"
    t.wakeup
    assert_equals(false, madeit) # Hasn't run it yet
    t.run
    t.join
    assert_equals(true, madeit)
  end

  def test_s_abort_on_exception
    assert_equal(false,Thread.abort_on_exception)
    Thread.abort_on_exception = true
    assert_equal(true,Thread.abort_on_exception)
    Thread.abort_on_exception = false
    assert_equal(false,Thread.abort_on_exception)
  end

  def test_s_abort_on_exception=
    save_stderr = nil

    begin
      Thread.new do
	raise "boom"
      end
      assert(true)
    rescue Exception
      fail("Thread exception propagated to main thread")
    end

    begin
      Thread.abort_on_exception = true
      Thread.new do
	save_stderr = $stderr
	$stderr = nil
	raise "boom"
      end
      fail("Exception should have interrupted main thread")
    rescue Exception
      assert(true)
    ensure
      Thread.abort_on_exception = false
      $stderr = save_stderr
    end
  end

  def test_s_critical
    assert_equal(false,Thread.critical)
    Thread.critical = true
    assert_equal(true,Thread.critical)
    Thread.critical = false
    assert_equal(false,Thread.critical)
  end

  def test_s_critical=
    count = 0
    a = nil
    thread_control do
      a = Thread.new { _signal; loop { count += 1; Thread.pass }}
      _wait
    end

    Thread.critical = true
    saved = count # Fixnum, will copy the value
    100000.times { |i| Math.sin(i) ** Math.tan(i/2) }
    assert_equal(saved, count)

    Thread.critical = false
    100000.times { |i| Math.sin(i) ** Math.tan(i/2) }
    assert(saved != count)
  end

  def test_s_current
    t = nil
    thread_control do
      t = Thread.new { _signal; Thread.stop }
      _wait
    end
    assert(Thread.current != t)
  end

  def test_s_exit
    t = Thread.new { Thread.exit }
    t.join
    assert_equals(t, t.exit)
    assert_equals(false, t.alive?)
    IO.popen("#$interpreter -e 'Thread.exit; puts 123'") do |p|
      assert_nil(p.gets)
    end
    assert_equals(0, $?)
  end

  def test_s_fork
    madeit = false
    t = Thread.fork { madeit = true }
    t.join
    assert_equals(true,madeit)
  end

  def test_s_kill
    count = 0
    t = Thread.new { loop { Thread.pass; count += 1 }}
    sleep .1
    saved = count
    Thread.kill(t)
    sleep .1
    t.join
    assert_equals(saved, count)
  end

  def test_s_list
    t = []
    100.times { t << Thread.new { Thread.stop } }
    assert_equals(101, Thread.list.length)
    t.each { |i| i.run; i.join }
    assert_equals(1, Thread.list.length)
  end

  def test_s_main
    t = nil
    thread_control do
      t = Thread.new { _signal; Thread.stop }
      _wait
    end
    assert_equals(Thread.main, Thread.current)
    assert(Thread.main != t)
  end

  def test_s_new
    madeit = false
    t = Thread.new { madeit = true }
    t.join
    assert_equals(true,madeit)
  end

  def test_s_pass
    madeit = false
    t = Thread.new { Thread.pass; madeit = true }
    t.join
    assert_equals(true, madeit)
  end

  def test_s_start
    t = nil
    thread_control do
      t = SubThread.new { _signal; Thread.stop }
      _wait
    end
    assert_equals(true, t.initCalled?)

    thread_control do
      t = SubThread.start { _signal; Thread.stop }
      _wait
    end
    assert_equals(nil, t.initCalled?)
  end

  def test_s_stop
    t = nil
    thread_control do
      t = Thread.new { Thread.critical = true; _signal; Thread.stop }
      _wait
    end
    assert_equals(false,   Thread.critical)
    assert_equals("sleep", t.status)
  end

end

Rubicon::handleTests(TestThread) if $0 == __FILE__
