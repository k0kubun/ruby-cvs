require '../rubicon'


class TestKernel < Rubicon::TestCase

  def test_EQUAL # '=='
    assert_fail("untested")
  end

  def test_MATCH # '=~'
    assert_fail("untested")
  end

  def test_VERY_EQUAL # '==='
    assert_fail("untested")
  end

  def test___id__
    assert_fail("untested")
  end

  def test___send__
    assert_fail("untested")
  end

  def test_class
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_display
    assert_fail("untested")
  end

  def test_dup
    assert_fail("untested")
  end

  def test_eql?
    assert_fail("untested")
  end

  def test_equal?
    assert_fail("untested")
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
    assert_fail("untested")
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
    assert_fail("untested")
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
    assert(!defined? Module_Test::VAL)
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
    begin
      eval "1
            burble", binding, "gumby", 321
    rescue Exception => detail
      puts "Error:"
      puts detail
    end
  end

  def test_s_exec
    assert_fail("untested")
  end

  def test_s_exit
    assert_fail("untested")
  end

  def test_s_exit!
    assert_fail("untested")
  end

  def test_s_fail
    assert_fail("untested")
  end

  def test_s_fork
    assert_fail("untested")
  end

  def test_s_format
    assert_fail("untested")
  end

  def test_s_getc
    assert_fail("untested")
  end

  def test_s_gets
    assert_fail("untested")
  end

  def test_s_global_variables
    assert_fail("untested")
  end

  def test_s_gsub
    assert_fail("untested")
  end

  def test_s_gsub!
    assert_fail("untested")
  end

  def test_s_iterator?
    assert_fail("untested")
  end

  def test_s_lambda
    assert_fail("untested")
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
    assert_fail("untested")
  end

  def test_s_putc
    assert_fail("untested")
  end

  def test_s_puts
    assert_fail("untested")
  end

  def test_s_raise
    assert_fail("untested")
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
