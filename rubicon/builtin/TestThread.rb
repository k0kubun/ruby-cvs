require '../rubicon'


class TestThread < Rubicon::TestCase

  class SubThread < Thread
    def initialize
      @wasCalled = true
      super
    end
    def initCalled?
      @wasCalled
    end
  end
  
  #
  # Signal
  #
  def s
    $spinlock = true
  end
  
  #
  # Wait
  #
  def w
    sleep .1 while !$spinlock
    $spinlock = false
  end

  def teardown
    Thread.list.each {|t|
      if t != Thread.main
        t.kill
      end
    }
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
    assert_equal(false,Thread.current.abort_on_exception)
    Thread.current.abort_on_exception = true
    assert_equal(true,Thread.current.abort_on_exception)
    Thread.current.abort_on_exception = false
    assert_equal(false,Thread.current.abort_on_exception)
  end

  def test_abort_on_exception=
    result = runChild { 
      Thread.new { 
        $stderr.close # Don't want to see the mess.
        Thread.new {
          Thread.current.abort_on_exception = true
          raise "Error"
        }
      }
      sleep 5
      exit 1
    }
    assert_equal(0,result) # Relies on abort doing exit(0)
  end

  def test_alive?
    t1 = Thread.new { s; Thread.stop }
    w
    t2 = Thread.new { s; sleep 60 }
    w
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

  def try_join(dojoin=false)
    a = []
    t = []
    10.times {|i|
      t[i] = Thread.new { 
        10.times {|j|
          a << "#{i}-#{j}"
          sleep .01
        }
      }
    }
    if (dojoin)
      10.times {|i| t[i].join }
    end
    a.length
  end

  def test_join

    result = runChild {
      exit try_join
    }
    assert(result != 100)

    result = runChild {
      exit try_join(true)
    }
    assert_equals(100, result)


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
    assert_equals(t,t.kill)
    assert_equals(false,t.alive?)
  end

  def test_priority
    assert_equals(0, Thread.current.priority)
  end

  def test_priority=
    c1 = 0
    c2 = 0
    a = Thread.new { loop { c1 += 1 }}
    b = Thread.new { loop { c2 += 1 }}
    a.priority = -2
    b.priority = -1
    sleep 1
    Thread.critical = true
    assert (c2 > c1)
    c1 = 0
    c2 = 0
    a.priority = -1
    b.priority = -2
    Thread.critical = false
    sleep 1 
    assert (c1 > c2)
    a.kill
    b.kill
  end

  def test_raise
    madeit = false
    t = Thread.new { s; sleep 60; madeit = true }
    w
    t.raise "Gotcha"
    assert(!t.alive?)
    assert_equals(false,madeit)
  end

  def test_run
    wokeup = false
    t1 = Thread.new { s; Thread.stop; wokeup = true }
    w
    assert_equals(false, wokeup)
    t1.run
    assert_equals(true, wokeup)

    wokeup = false
    t1 = Thread.new { s; Thread.stop; wokeup = true }
    w
    assert_equals(false, wokeup)
    Thread.critical = true
    t1.run
    assert_equals(false, wokeup)
    Thread.critical = false
    t1.run
    assert_equals(true, wokeup)
  end

  def test_safe_level
    result = runChild {
      $stderr.close
      exit 1 if 0 != Thread.current.safe_level
      $SAFE=1
      exit 1 if 1 != Thread.current.safe_level
      exit 0
    }
    assert_equals(0, result)
  end

  def test_status
    a = Thread.new { s; raise "dead" }
    w
    b = Thread.new { s; Thread.stop }
    w
    c = Thread.new { s; Thread.exit }
    w
    assert_equals("run", Thread.current.status)
    assert_equals(nil, a.status)
    assert_equals("sleep", b.status)
  end

  def test_stop?
    a = Thread.new { s; Thread.stop }
    w
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
    t = Thread.new { s; Thread.stop; madeit = true }
    w
    assert_equals(false, madeit)
    t.wakeup
    assert_equals(false, madeit) # Hasn't run it yet
    t.run
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
    result = runChild { 
      Thread.new { 
        $stderr.close # Don't want to see the mess.
        Thread.abort_on_exception = true
        Thread.new { s; raise "Error" }
        w
      }
      sleep 5
      exit 1
    }
    assert_equal(0,result) # Relies on abort doing exit(0)
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
    a = Thread.new { s; loop { count += 1; sleep .01 }}
    w
    Thread.critical = true
    saved = count # Fixnum, will copy the value
    foo = 0
    10000.times { |i| foo += Math.sin(i) ** Math.tan(i/2) }
    assert_equal(saved, count)
    Thread.critical = false
    10000.times { |i| foo += Math.sin(i) ** Math.tan(i/2) }
    assert(saved != count)
  end

  def test_s_current
    t = Thread.new { s; Thread.stop }
    w
    assert(Thread.current != t)
  end

  def test_s_exit
    t = Thread.new { Thread.exit }
    t.join
    assert_equals(t,t.exit)
    assert_equals(false,t.alive?)
    result = runChild {
      Thread.exit
      exit(1)
    }
    assert_equals(0,result)
  end

  def test_s_fork
    madeit = false
    t = Thread.fork { madeit = true }
    t.join
    assert_equals(true,madeit)
  end

  def test_s_kill
    count = 0
    t = Thread.new { loop { count += 1 }}
    sleep .1
    saved = count
    Thread.kill(t)
    sleep .1
    t.join
    assert_equals(saved,count)
  end

  def test_s_list
    t = []
    100.times { |i| t[i] = Thread.new { sleep 1 } }
    assert_equals(101,Thread.list.length)
    100.times { |i| t[i].join }
    assert_equals(1,Thread.list.length)
  end

  def test_s_main
    t = Thread.new { s; Thread.stop }
    w
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
    t = Thread.new { s; Thread.pass; madeit = true }
    w
    assert_equals(false, madeit)
    t.join
    assert_equals(true, madeit)
  end

  def test_s_start
    t = SubThread.new { s; Thread.stop }
    w
    assert_equals(true,t.initCalled?)
    t = SubThread.start { s; Thread.stop }
    w
    assert_equals(nil,t.initCalled?)
  end

  def test_s_stop
    t = Thread.new { s; Thread.critical = true; Thread.stop }
    w
    assert_equals(false,Thread.critical)
    assert_equals("sleep",t.status)
  end

end

Rubicon::handleTests(TestThread) if $0 == __FILE__
