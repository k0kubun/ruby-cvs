#
#		thread.rb - thread support classes
#			$Date$
#			by Yukihiro Matsumoto <matz@caelum.co.jp>
#

unless defined? Thread
  fail "Thread not available for this ruby interpreter"
end

unless defined? ThreadError
  class ThreadError<StandardError
  end
end

if $DEBUG
  Thread.abort_on_exception = true
end

def Thread.exclusive
  begin
    Thread.critical = true
    r = yield
  ensure
    Thread.critical = false
  end
  r
end

class Mutex
  def initialize
    @waiting = []
    @locked = false;
    @waiting.taint		# enable tainted comunication
    self.taint
  end

  def locked?
    @locked
  end

  def try_lock
    result = false
    Thread.critical = true
    unless @locked
      @locked = true
      result = true
    end
    Thread.critical = false
    result
  end

  def lock
    while (Thread.critical = true; @locked)
      @waiting.push Thread.current
      Thread.stop
    end
    @locked = true
    Thread.critical = false
    self
  end

  def unlock
    return unless @locked
    Thread.critical = true
    t = @waiting.shift
    @locked = false
    Thread.critical = false
    t.run if t
    self
  end

  def synchronize
    lock
    begin
      yield
    ensure
      unlock
    end
  end

  def exclusive_unlock
    return unless @locked
    Thread.exclusive do
      t = @waiting.shift
      @locked = false
      t.wakeup if t
      yield
    end
    self
  end
end

class ConditionVariable
  def initialize
    @waiters = []
  end
  
  def wait(mutex)
    mutex.exclusive_unlock do
      @waiters.push(Thread.current)
      Thread.stop
    end
    mutex.lock
  end
  
  def signal
    t = @waiters.shift
    t.run if t
  end
    
  def broadcast
    waiters0 = nil
    Thread.exclusive do
      waiters0 = @waiters.dup
      @waiters.clear
    end
    for t in waiters0
      t.run
    end
  end
end

class Queue
  def initialize
    @que = []
    @waiting = []
    @que.taint		# enable tainted comunication
    @waiting.taint
    self.taint
  end

  def push(obj)
    Thread.critical = true
    @que.push obj
    t = @waiting.shift
    Thread.critical = false
    t.run if t
  end
  alias enq push

  def pop non_block=false
    Thread.critical = true
    begin
      loop do
	if @que.length == 0
	  if non_block
	    raise ThreadError, "queue empty"
	  end
	  @waiting.push Thread.current
	  Thread.stop
	else
	  return @que.shift
	end
      end
    ensure
      Thread.critical = false
    end
  end
  alias shift pop
  alias deq pop

  def empty?
    @que.length == 0
  end

  def clear
    @que.replace([])
  end

  def length
    @que.length
  end
  alias size length


  def num_waiting
    @waiting.size
  end
end

class SizedQueue<Queue
  def initialize(max)
    @max = max
    @queue_wait = []
    @queue_wait.taint		# enable tainted comunication
    super()
  end

  def max
    @max
  end

  def max=(max)
    Thread.critical = true
    if @max >= max
      @max = max
      Thread.critical = false
    else
      diff = max - @max
      @max = max
      Thread.critical = false
      diff.times do
	t = @queue_wait.shift
	t.run if t
      end
    end
    max
  end

  def push(obj)
    Thread.critical = true
    while @que.length >= @max
      @queue_wait.push Thread.current
      Thread.stop
      Thread.critical = true
    end
    super
  end

  def pop(*args)
    Thread.critical = true
    if @que.length < @max
      t = @queue_wait.shift
      t.run if t
    end
    super
  end

  def num_waiting
    @waiting.size + @queue_wait.size
  end
end
