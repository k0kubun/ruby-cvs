require '../rubicon'


class TestKernel < Rubicon::TestCase

  def test_EQUAL # '=='
    o1 = Object.new
    o2 = Object.new
    assert(o1 == o1)
    assert(o2 == o2)
    assert(o1 != o2)
    assert(!(o1 == o2))
  end

  def test_MATCH # '=~'
    o1 = Object.new
    o2 = Object.new
    assert(!(o1 =~ o1))
    assert(!(o2 =~ o2))
    assert(o1 !~ o2)
    assert(!(o1 =~ o2))
  end

  def test_VERY_EQUAL # '==='
    o1 = Object.new
    o2 = Object.new
    assert(o1 === o1)
    assert(o2 === o2)
    assert(!(o1 === o2))
  end

  def test___id__
    # naive test - no 2 ids the same
    objs = []
    ObjectSpace.each_object { |obj| objs << obj.__id__ }
    s1 = objs.size
    assert_equal(s1, objs.uniq.size)

    assert_equal(1.__id__, (3-2).__id__)
    assert_instance_of(Fixnum, 1.__id__)
  end

  class SendTest
    def send_test1
      "send1"
    end
    
    def send_test2(a, b)
      a + b
    end
  end

  def test___send__
    t = SendTest.new
    assert_equal("send1", t.__send__(:send_test1))
    assert_equal(99,      t.__send__("send_test2", 44, 55))
  end

  def test_class
    assert_instance_of(Class, 1.class)
    assert_equal(Fixnum, 1.class)
    assert_equal(Class, TestKernel.class)
    assert_equal(Class, TestKernel.class.class)
    assert_equal(Module, Enumerable.class)
  end

  class CloneTest
    attr_accessor :str
  end

  def test_clone
    s1 = CloneTest.new
    s1.str = "hello"
    s2 = s1.clone
    assert(s1.str    == s2.str)
    assert(s1.str.id == s2.str.id)
    assert(s1.id     != s2.id)
  end

  class DisplayTest
    attr :val
    def write(obj)
      @val = "!#{obj.to_s}!"
    end
  end

  def test_display
    dt = DisplayTest.new
    assert_nil("hello".display(dt))
    assert_equal("!hello!", dt.val)

    save = $>
    begin
      $> = dt
      assert_nil("hello".display)
      assert_equal("!hello!", dt.val)
    rescue
      $> = save
    end
  end

  def test_dup
    s1 = CloneTest.new
    s1.str = "hello"
    s2 = s1.dup
    assert(s1.str    == s2.str)
    assert(s1.str.id == s2.str.id)
    assert(s1.id     != s2.id)
  end

  def test_eql?
    o1 = Object.new
    o2 = Object.new
    assert(o1.eql? o1)
    assert(o2.eql? o2)
    assert(!(o1.eql? o2))
  end

  def test_equal?
    o1 = Object.new
    o2 = Object.new
    assert(o1.equal? o1)
    assert(o2.equal? o2)
    assert(!(o1.equal? o2))
  end

  def test_extend
    assert_fail("untested")
  end

  def test_freeze
    assert_fail("untested")
  end

  def test_frozen?
    assert_fail("untested")
  end

  def test_hash
    assert_fail("untested")
  end

  def test_id
    objs = []
    ObjectSpace.each_object { |obj| objs << obj.id }
    s1 = objs.size
    assert_equal(s1, objs.uniq.size)

    assert_equal(1.id, (3-2).id)
    assert_instance_of(Fixnum, 1.id)
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_instance_eval
    assert_fail("untested")
  end

  def test_instance_of?
    assert_fail("untested")
  end

  def test_instance_variables
    assert_fail("untested")
  end

  def test_is_a?
    assert_fail("untested")
  end

  def test_kind_of?
    assert_fail("untested")
  end

  def test_method
    assert_fail("untested")
  end

  def test_methods
    assert_fail("untested")
  end

  def test_nil?
    assert_fail("untested")
  end

  def test_private_methods
    assert_fail("untested")
  end

  def test_protected_methods
    assert_fail("untested")
  end

  def test_public_methods
    assert_fail("untested")
  end

  def test_respond_to?
    assert_fail("untested")
  end

  def test_send
    t = SendTest.new
    assert_equal("send1", t.send(:send_test1))
    assert_equal(99,      t.send("send_test2", 44, 55))
  end

  def test_singleton_methods
    assert_fail("untested")
  end

  def test_taint
    assert_fail("untested")
  end

  def test_tainted?
    assert_fail("untested")
  end

  def test_to_a
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_type
    assert_fail("untested")
  end

  def test_untaint
    assert_fail("untested")
  end

  class Caster
    def to_a
      [4, 5, 6]
    end
    def to_f
      2.34
    end
    def to_i
      99
    end
    def to_s
      "Hello"
    end
  end

  def test_s_Array
    assert_equal([], Array(nil))
    assert_equal([1, 2, 3], Array([1, 2, 3]))
    assert_equal([1, 2, 3], Array(1..3))
    assert_equal([4, 5, 6], Array(Caster.new))
  end

  def test_s_Float
    assert_instance_of(Float, Float(1))
    assert_flequal(1, Float(1))
    assert_flequal(1.23, Float('1.23'))
    assert_flequal(2.34, Float(Caster.new))
  end

  def test_s_Integer
    assert_instance_of(Fixnum, Integer(1))
    assert_instance_of(Bignum, Integer(10**30))
    assert_equal(123,    Integer(123.99))
    assert_equal(-123,   Integer(-123.99))
    a = Integer(1.0e30) - 10**30
    assert(a.abs < 10**20)
    assert_equal(99, Integer(Caster.new))
  end

  def test_s_String
    assert_instance_of(String, String(123))
    assert_equal("123", String(123))
    assert_equal("123.45", String(123.45))
    assert_equal("123",    String([1, 2, 3]))
    assert_equal("Hello",    String(Caster.new))
  end

  def test_s_BACKTICK
    assert_equal("hello\n", `echo hello`)
    assert_equal(0, $?)
    assert_equal("", `not_a_valid_command 2>/dev/null`)
    assert($? != 0)

    assert_equal("hello\n", %x{echo hello})
    assert_equal(0, $?)
    assert_equal("", %x{not_a_valid_command 2>/dev/null})
    assert($? != 0)
  end

  def test_s_abort
    f = fork
    if !f
      abort
      exit 99                   # should not get here
    else
      Process.wait
      assert_equal(1<<8, $?)
    end
  end

  def test_s_at_exit
    p = IO.popen("-")

    if !p
      at_exit { puts "world" }
      at_exit { puts "cruel" }
      puts "goodbye"
      exit 99
    else
      assert_equal("goodbye\n", p.gets)
      assert_equal("cruel\n", p.gets)
      assert_equal("world\n", p.gets)
      p.close
      assert_equal(99<<8, $?)
    end
  end

  def test_s_autoload
    File.open("_dummy.rb", "w") do |f|
     f.print <<-EOM
      module Module_Test
        VAL = 123
      end
      EOM
    end
    assert(!defined? Module_Test)
    autoload(:Module_Test, "./_dummy.rb")
    assert(defined? Module_Test::VAL)
    assert_equal(123, Module_Test::VAL)
    assert($".include?("./_dummy.rb"))#"
    File.delete("./_dummy.rb")
  end

  def bindproc(val)
    return binding
  end

  def test_s_binding
    val = 123
    b = binding
    assert_instance_of(Binding, b)
    assert_equal(123, eval("val", b))
    b = bindproc(321)
    assert_equal(321, eval("val", b))
  end

  def blocker1
    return block_given?
  end

  def blocker2(&p)
    return block_given?
  end

  def test_s_block_given?
    assert(!blocker1())
    assert(!blocker2())
    assert(blocker1() { 1 })
    assert(blocker2() { 1 })
  end

  def test_s_callcc
    assert_fail("untested")
  end

  def caller_test
    return caller
  end

  def test_s_caller
    c = caller_test
    assert_equal(%{TestKernel.rb:#{__LINE__-1}:in `test_s_caller'},
                c[0])
  end

  def catch_test
    throw :c, 456;
    789;
  end

  def test_s_catch
    assert_equal(123, catch(:c) { a = 1; 123 })
    assert_equal(321, catch(:c) { a = 1; throw :c, 321; 123 })
    assert_equal(456, catch(:c) { a = 1; catch_test; 123 })

    assert_equal(456, catch(:c) { catch (:d) { catch_test; 123 } } )
  end

  def test_s_chomp
    $_ = "hello"
    assert_equal("hello", chomp)
    assert_equal("hello", chomp('aaa'))
    assert_equal("he",    chomp('llo'))
    assert_equal("he",    $_)

    a  = "hello\n"
    $_ = a
    assert_equal("hello",   chomp)
    assert_equal("hello",   $_)
    assert_equal("hello\n", a)
  end

  def test_s_chomp!
    $_ = "hello"
    assert_equal(nil,     chomp!)
    assert_equal(nil,     chomp!('aaa'))
    assert_equal("he",    chomp!('llo'))
    assert_equal("he",    $_)

    a  = "hello\n"
    $_ = a
    assert_equal("hello", chomp!)
    assert_equal("hello", $_)
    assert_equal("hello", a)
  end

  def test_s_chop
    a  = "hi"
    $_ = a
    assert_equal("h",  chop)
    assert_equal("h",  $_)
    assert_equal("",   chop)
    assert_equal("",   $_)
    assert_equal("",   chop)
    assert_equal("hi", a)

    $_ = "hi\n"
    assert_equal("hi", chop)

    $_ = "hi\r\n"
    assert_equal("hi", chop)
    $_ = "hi\n\r"
    assert_equal("hi\n", chop)
  end

  def test_s_chop!
    a  = "hi"
    $_ = a
    assert_equal("h",  chop!)
    assert_equal("h",  $_)
    assert_equal("",   chop!)
    assert_equal("",   $_)
    assert_equal(nil,   chop!)
    assert_equal("",   $_)
    assert_equal("", a)

    $_ = "hi\n"
    assert_equal("hi", chop!)

    $_ = "hi\r\n"
    assert_equal("hi", chop!)
    $_ = "hi\n\r"
    assert_equal("hi\n", chop!)
  end

  def test_s_eval
    assert_equal(123, eval("100 + 20 + 3"))
    val = 123
    assert_equal(123, eval("val", binding))
    assert_equal(321, eval("val", bindproc(321)))
    skipping("Check of eval with file name")
    begin
      eval "1
            burble", binding, "gumby", 321
    rescue Exception => detail
    end
  end

  def test_s_exec
    p = IO.popen("-")
    if p.nil?
      exec "echo TestKer*.rb"
    else
      begin
        assert_equal("TestKernel.rb\n", p.gets)
      ensure
        p.close
      end
    end

    # With separate parameters, don't do expansion
    p = IO.popen("-")
    if p.nil?
      exec "echo", "TestKer*.rb"
    else
      begin
        assert_equal("TestKer*.rb\n", p.gets)
      ensure
        p.close
      end
    end
  end

  def test_s_exit
    begin
      exit
      assert_fail("No exception raised")
    rescue SystemExit
      assert(true)
    rescue Exception
      assert_fail("Bad exception: #$!")
    end

    f = fork
    if f.nil?
      exit
    end
    Process.wait
    assert_equal(0, $?)

    f = fork
    if f.nil?
      exit 123
    end
    Process.wait
    assert_equal(123 << 8, $?)
  end

  def test_s_exit!
    f = fork

    if f.nil?
      begin
        exit! 99
        exit 1
      rescue SystemExit
        exit 2
      rescue Exception
        exit 3
      end
      exit 4
    end
    Process.wait
    assert_equal(99<<8, $?)

    f = fork
    if f.nil?
      exit!
    end
    Process.wait
    assert_equal(0xff << 8, $?)

    f = fork
    if f.nil?
      exit! 123
    end
    Process.wait
    assert_equal(123 << 8, $?)
  end

  def test_s_fail
    assert_exception(StandardError) { fail }
    assert_exception(StandardError) { fail "Wombat" }
    assert_exception(NotImplementError) { fail NotImplementError }

    begin
      fail "Wombat"
      assert_fail("No exception")
    rescue StandardError => detail
      assert_equal("Wombat", detail.message)
    rescue Exception
      assert_fail("Wrong exception")
    end

    begin
      fail NotImplementError, "Wombat"
      assert_fail("No exception")
    rescue NotImplementError => detail
      assert_equal("Wombat", detail.message)
    rescue Exception
      raise
    end

    bt = %w(one two three)
    begin

      fail NotImplementError, "Wombat", bt
      assert_fail("No exception")
    rescue NotImplementError => detail
      assert_equal("Wombat", detail.message)
      puts
      assert_equal(bt, detail.backtrace)
    rescue Exception
      raise
    end

  end

  def test_s_fork
    f = fork
    if f.nil?
      File.open("_pid", "w") {|f| f.puts $$}
      exit 99
    end
    begin
      Process.wait
      assert_equal(99<<8, $?)
      File.open("_pid") do |file|
        assert_equal(file.gets.to_i, f)
      end
    ensure
      File.delete("_pid")
    end

    f = fork do
      File.open("_pid", "w") {|f| f.puts $$}
    end
    begin
      Process.wait
      assert_equal(0<<8, $?)
      File.open("_pid") do |file|
        assert_equal(file.gets.to_i, f)
      end
    ensure
      File.delete("_pid")
    end
  end

  def test_s_format
    assert_equals("00123", format("%05d", 123))
    assert_equals("123  |00000001", format("%-5s|%08x", 123, 1))
    x = format("%3s %-4s%%foo %.0s%5d %#x%c%3.1f %b %x %X %#b %#x %#X",
      "hi",
      123,
      "never seen",
      456,
      0,
      ?A,
      3.0999,
      11,
      171,
      171,
      11,
      171,
      171)

    assert_equal(' hi 123 %foo   456 0x0A3.1 1011 ab AB 0b1011 0xab 0XAB', x)
  end

  def setupFiles
    setupTestDir
    File.open("_test/_file1", "w") do |f|
      f.puts "0: Line 1"
      f.puts "1: Line 2"
    end
    File.open("_test/_file2", "w") do |f|
      f.puts "2: Line 1"
      f.puts "3: Line 2"
    end
    ARGV.replace ["_test/_file1", "_test/_file2" ]
  end

  def teardownFiles
    teardownTestDir
  end

  def test_s_gets1
    setupFiles
    begin
      count = 0
      while gets
        num = $_[0..1].to_i
        assert_equal(count, num)
        count += 1
      end
      assert_equal(4, count)
    ensure
      teardownFiles
    end
  end

  def test_s_gets2      
    setupFiles
    begin
      count = 0
      while gets(nil)
        split(/\n/).each do |line|
          num = line[0..1].to_i
          assert_equal(count, num)
          count += 1
        end
      end
      assert_equal(4, count)
    ensure
      teardownFiles
    end
  end

  def test_s_gets3
    setupFiles
    begin
      count = 0
      while gets(' ')
        count += 1
      end
      assert_equal(10, count)
    ensure
      teardownFiles
    end
  end

  def test_s_global_variables
    g1 = global_variables
    assert_instance_of(Array, g1)
    assert_instance_of(String, g1[0])
    assert(!g1.include?("$fred"))
    eval "$fred = 1"
    g2 = global_variables
    assert(g2.include?("$fred"))
    assert_equal(["$fred"], g2 - g1)
  end

  def test_s_gsub
    $_ = "hello"
    assert_equal("h*ll*", gsub(/[aeiou]/, '*'))
    assert_equal("h*ll*", $_)

    $_ = "hello"
    assert_equal("h<e>ll<o>", gsub(/([aeiou])/, '<\1>'))
    assert_equal("h<e>ll<o>", $_)

    $_ = "hello"
    assert_equal("104 101 108 108 111 ", gsub('.') {
                   |s| s[0].to_s+' '})
    assert_equal("104 101 108 108 111 ", $_)

    $_ = "hello"
    assert_equal("HELL-o", gsub(/(hell)(.)/) {
                   |s| $1.upcase + '-' + $2
                   })
    assert_equal("HELL-o", $_)

    $_ = "hello"
    $_.taint
    assert_equal(true, (gsub('.','X').tainted?))
    assert_equal(true, $_.tainted?)
  end

  def test_s_gsub!
    $_ = "hello"
    assert_equal("h*ll*", gsub!(/[aeiou]/, '*'))
    assert_equal("h*ll*", $_)

    $_ = "hello"
    assert_equal("h<e>ll<o>", gsub!(/([aeiou])/, '<\1>'))
    assert_equal("h<e>ll<o>", $_)

    $_ = "hello"
    assert_equal("104 101 108 108 111 ", gsub!('.') {
                   |s| s[0].to_s+' '})
    assert_equal("104 101 108 108 111 ", $_)

    $_ = "hello"
    assert_equal("HELL-o", gsub!(/(hell)(.)/) {
                   |s| $1.upcase + '-' + $2
                   })
    assert_equal("HELL-o", $_)

    $_ = "hello"
    assert_equal(nil, gsub!(/x/, 'y'))
    assert_equal("hello", $_)

    $_ = "hello"
    $_.taint
    assert_equal(true, (gsub!('.','X').tainted?))
    assert_equal(true, $_.tainted?)
  end

  def iterator_test(&b)
    return iterator?
  end
    
  def test_s_iterator?
    assert(iterator_test { 1 })
    assert(!iterator_test)
  end

  def test_s_lambda
    a = lambda { "hello" }
    assert_equal("hello", a.call)
    a = lambda { |s| "there " + s  }
    assert_equal("there Dave", a.call("Dave"))
  end

  def test_s_load
    assert_fail("untested")
  end

  def test_s_local_variables
    assert_fail("untested")
  end

  def test_s_loop
    assert_fail("untested")
  end

  def test_s_method_missing
    assert_fail("untested")
  end

  def test_s_open
    assert_fail("untested")
  end

  def test_s_p
    assert_fail("untested")
  end

  def test_s_print
    assert_fail("untested")
  end

  def test_s_printf
    assert_fail("untested")
  end

  def test_s_proc
    a = proc { "hello" }
    assert_equal("hello", a.call)
    a = proc { |s| "there " + s  }
    assert_equal("there Dave", a.call("Dave"))
  end

  def test_s_putc
    assert_fail("untested")
  end

  def test_s_puts
    assert_fail("untested")
  end

  def test_s_raise
    assert_exception(StandardError) { raise }
    assert_exception(StandardError) { raise "Wombat" }
    assert_exception(NotImplementError) { raise NotImplementError }

    begin
      raise "Wombat"
      assert_fail("No exception")
    rescue StandardError => detail
      assert_equal("Wombat", detail.message)
    rescue Exception
      assert_fail("Wrong exception")
    end

    begin
      raise NotImplementError, "Wombat"
      assert_fail("No exception")
    rescue NotImplementError => detail
      assert_equal("Wombat", detail.message)
    rescue Exception
      raise
    end

    bt = %w(one two three)
    begin

      raise NotImplementError, "Wombat", bt
      assert_fail("No exception")
    rescue NotImplementError => detail
      assert_equal("Wombat", detail.message)
      puts
      assert_equal(bt, detail.backtrace)
    rescue Exception
      raise
    end
  end

  def test_s_rand
    assert_fail("untested")
  end

  def test_s_readline
    assert_fail("untested")
  end

  def test_s_readlines
    assert_fail("untested")
  end

  def test_s_require
    assert_fail("untested")
  end

  def test_s_scan
    assert_fail("untested")
  end

  def test_s_select
    assert_fail("untested")
  end

  def test_s_set_trace_func
    assert_fail("untested")
  end

  def test_s_singleton_method_added
    assert_fail("untested")
  end

  def test_s_sleep
    assert_fail("untested")
  end

  def test_s_split
    assert_fail("untested")
  end

  def test_s_sprintf
    assert_fail("untested")
  end

  def test_s_srand
    assert_fail("untested")
  end

  def test_s_sub
    assert_fail("untested")
  end

  def test_s_sub!
    assert_fail("untested")
  end

  def test_s_syscall
    assert_fail("untested")
  end

  def test_s_system
    assert_fail("untested")
  end

  def test_s_test
    assert_fail("untested")
  end

  def test_s_throw
    assert_fail("untested")
  end

  def test_s_trace_var
    assert_fail("untested")
  end

  def test_s_trap
    assert_fail("untested")
  end

  def test_s_untrace_var
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestKernel) if $0 == __FILE__
