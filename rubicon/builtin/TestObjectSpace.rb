$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestObjectSpace < Rubicon::TestCase

  WORKED = "It Worked!\n"

  def test_s__id2ref
    s = "hello"
    t = ObjectSpace._id2ref(s.id)
    assert_equal(s, t)
    assert_equal(s.id, t.id)
  end


  # finalizer manipulation
  def test_s_finalizers
    tf = Tempfile.new("tf")
    begin
      tf.puts %{
	a = "string"
	ObjectSpace.define_finalizer(a) { puts(WORKED) }
	a = "another string"
	GC.start
	exit}
      tf.close
      p = IO.popen("#$interpreter #{tf.path}")
      begin
	assert_equal(WORKED, p.gets)
      ensure
	p.close
      end
    ensure
      tf.close(true)
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
