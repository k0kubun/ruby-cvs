require '../rubicon'


class TestProc < Rubicon::TestCase

  def procFrom
    Proc.new
  end

  def test_AREF # '[]'
    a = Proc.new { |x| "hello #{x}" }
    assert_equal("hello there", a["there"])
  end

  def test_arity
    a = Proc.new { "hello" }
    assert_equal(-1, a.arity)
    a = Proc.new { |x| "hello #{x}" }
    assert_equal(-2, a.arity)
    a = Proc.new { |x,y| "hello #{x}" }
    assert_equal(2, a.arity)
    a = Proc.new { |x,y,z| "hello #{x}" }
    assert_equal(3, a.arity)
    a = Proc.new { |x,*z| "hello #{x}" }
    assert_equal(-2, a.arity)
    a = Proc.new { |*x| "hello #{x}" }
    assert_equal(-1, a.arity)
  end

  def test_call
    a = Proc.new { |x| "hello #{x}" }
    assert_equal("hello there", a.call("there"))
  end

  def test_s_new
    a = procFrom { "hello" }
    assert_equal("hello", a.call)
    a = Proc.new { "there" }
    assert_equal("there", a.call)
  end

end

Rubicon::handleTests(TestProc) if $0 == __FILE__
