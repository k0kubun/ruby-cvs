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
    tests = [
      [Proc.new {          }, -1],
      [Proc.new { |x,y|    },  2],
      [Proc.new { |x,y,z|  },  3],
      [Proc.new { |*z|     }, -1],
      [Proc.new { |x,*z|   }, -2],
      [Proc.new { |x,y,*z| }, -3],
    ]

    if $rubyVersion <= "1.6.1"
      tests << 
        [Proc.new { ||       },  -1] <<
        [Proc.new { |x|      },  -2]
    else
      tests <<
        [Proc.new { ||       },  0] <<
        [Proc.new { |x|      }, -1]
    end

    tests.each do |proc, expected_arity|
      assert_equal(expected_arity, proc.arity)
    end
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
