require '../rubicon'


class TestObjectSpace < Rubicon::TestCase

  def test_s__id2ref
    s = "hello"
    t = ObjectSpace._id2ref(s.id)
    assert_equal(s, t)
    assert_equal(s.id, t.id)
  end

  # finalizer manipulation
  def test_s_finalizers
    p1 = proc {|o| 1; }
    p2 = proc {|o| 2; }
    assert_set_equal([], ObjectSpace.finalizers)
    ObjectSpace.add_finalizer(p1)
    assert_set_equal([p1], ObjectSpace.finalizers)
    ObjectSpace.add_finalizer(p2)
    assert_set_equal([p1, p2], ObjectSpace.finalizers)
    ObjectSpace.remove_finalizer(p2)
    assert_set_equal([p1], ObjectSpace.finalizers)
    ObjectSpace.remove_finalizer(p1)
    assert_set_equal([], ObjectSpace.finalizers)
  end

  def test_s_call_finalizer
    IO.popen("-") do |pipe|
      if !pipe
        ObjectSpace.add_finalizer(proc {|id| puts "f: #{id}" })
        a = "a"
        b = "b"
        c = "c"
        puts a.id
        puts c.id
        ObjectSpace.call_finalizer(a)
        ObjectSpace.call_finalizer(c)
        exit
      end
      expected = []
      2.times do
        id = pipe.gets
        expected << "f: #{id}"
      end

      result = pipe.readlines
      assert_set_equal(expected, result)
    end
  end

  class A;      end
  class B < A;  end
  class C < A;  end
  class D < C;  end

  def test_s_each_object
    a = A.new
    b = B.new
    c = C.new
    d = D.new

    res = []
    ObjectSpace.each_object(TestObjectSpace::A) { |o| res << o }
    assert_set_equal([a, b, c, d], res)

    res = []
    ObjectSpace.each_object(B) { |o| res << o }
    assert_set_equal([b], res)

    res = []
    ObjectSpace.each_object(C) { |o| res << o }
    assert_set_equal([c, d], res)
  end

  def test_s_garbage_collect
    skipping("how to test")
  end

#  Tested in finalizers
#  def test_s_finalizers
#    assert_fail("untested")
#  end

# Tested in finalizers
#  def test_s_remove_finalizer
#    assert_fail("untested")
#  end

end

Rubicon::handleTests(TestObjectSpace) if $0 == __FILE__
