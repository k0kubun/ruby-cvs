require '../rubicon'


class TestModule < Rubicon::TestCase

  def test_CMP # '<=>'
    assert_fail("untested")
  end

  def test_GE # '>='
    assert_fail("untested")
  end

  def test_GT # '>'
    assert_fail("untested")
  end

  def test_LE # '<='
    assert_fail("untested")
  end

  def test_LT # '<'
    assert_fail("untested")
  end

  def test_VERY_EQUAL # '==='
    assert_fail("untested")
  end

  def test_ancestors
    assert_fail("untested")
  end

  def test_class_eval
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_const_defined?
    assert_fail("untested")
  end

  def test_const_get
    assert_fail("untested")
  end

  def test_const_set
    assert_fail("untested")
  end

  def test_constants
    assert_fail("untested")
  end

  def test_included_modules
    assert_fail("untested")
  end

  def test_instance_methods
    assert_fail("untested")
  end

  def test_method_defined?
    assert_fail("untested")
  end

  def test_module_eval
    assert_fail("untested")
  end

  def test_name
    assert_fail("untested")
  end

  def test_private_class_method
    assert_fail("untested")
  end

  def test_private_instance_methods
    assert_fail("untested")
  end

  def test_protected_instance_methods
    assert_fail("untested")
  end

  def test_public_class_method
    assert_fail("untested")
  end

  def test_public_instance_methods
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_s_constants
    assert_fail("untested")
  end

  def test_s_nesting
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestModule) if $0 == __FILE__
