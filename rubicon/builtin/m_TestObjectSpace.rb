require '../rubicon'


class TestObjectSpace < Rubicon::TestCase

  def test_s__id2ref
    assert_fail("untested")
  end

  def test_s_add_finalizer
    assert_fail("untested")
  end

  def test_s_call_finalizer
    assert_fail("untested")
  end

  def test_s_each_object
    assert_fail("untested")
  end

  def test_s_finalizers
    assert_fail("untested")
  end

  def test_s_garbage_collect
    assert_fail("untested")
  end

  def test_s_remove_finalizer
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestObjectSpace) if $0 == __FILE__
