require '../rubicon'

$m0 = Module.nesting

class TestModule < Rubicon::TestCase

  module Mixin
    MIXIN = 1
  end
  module User
    USER = 2
    include Mixin
  end
  module Other
  end

  def test_CMP # '<=>'
    assert_equal( 0, Mixin <=> Mixin)
    assert_equal(-1, User <=> Mixin)
    assert_equal( 1, Mixin <=> User)
    assert_equal( 1, Mixin <=> Other)

    assert_equal( 0, Object <=> Object)
    assert_equal(-1, String <=> Object)
    assert_equal( 1, Object <=> String)
  end

  def test_GE # '>='
    assert(Mixin >= User)
    assert(Mixin >= Mixin)
    assert(!(User >= Mixin))

    assert(Object >= String)
    assert(String >= String)
    assert(!(String >= Object))
  end

  def test_GT # '>'
    assert(Mixin   > User)
    assert(!(Mixin > Mixin))
    assert(!(User  > Mixin))

    assert(Object > String)
    assert(!(String > String))
    assert(!(String > Object))
  end

  def test_LE # '<='
    assert(User <= Mixin)
    assert(Mixin <= Mixin)
    assert(!(Mixin <= User))

    assert(String <= Object)
    assert(String <= String)
    assert(!(Object <= String))
  end

  def test_LT # '<'
    assert(User <= Mixin)
    assert(!(Mixin <= Mixin))
    assert(!(Mixin <= User))

    assert(String <= Object)
    assert(!(String <= String))
    assert(!(Object <= String))
  end

  def test_VERY_EQUAL # '==='
    assert(Object === self)
    assert(Rubicon::TestCase === self)
    assert(TestModule === self)
    assert(!(String === self))
  end

  def test_ancestors
    assert_equal([User, Mixin],      User.ancestors)
    assert_equal([Mixin],            Mixin.ancestors)

    assert_equal([Object, Kernel],   Object.ancestors)
    assert_equal([String, 
                   Enumerable, 
                   Comparable,
                   Object, Kernel],  String.ancestors)
  end

  def test_class_eval
    User.class_eval("CLASS_EVAL = 1")
    assert_equal(1, User::CLASS_EVAL)
    assert_equal(["CLASS_EVAL"], User.constants)
  end

  def test_const_defined?
    assert(Math.const_defined?(:PI))
    assert(Math.const_defined?("PI"))
    assert(!Math.const_defined?(:IP))
    assert(!Math.const_defined?("IP"))
  end

  def test_const_get
    assert_equal(Math::PI, Math.const_get("PI"))
    assert_equal(Math::PI, Math.const_get(:PI))
  end

  def test_const_set
    assert(!Other.const_defined?(:WOMBAT))
    Other.const_set(:WOMBAT, 99)
    assert(Other.const_defined?(:WOMBAT))
    assert_equal(99, Other::WOMBAT)
    save = $-v
    $-v = false
    Other.const_set("WOMBAT", "Hi")
    assert_equal("Hi", Other::WOMBAT)
    $-v = save
  end

  def test_constants
    assert_equal(["MIXIN"], Mixin.constants)
    assert_equal(["USER", "MIXIN"], User.constants)
  end

  def test_included_modules
    assert_equal([], Mixin.included_modules)
    assert_equal([Mixin], User.included_modules)
    assert_equal([Kernel], Object.included_modules)
    assert_equal([Enumerable, Comparable, Kernel], String.included_modules)
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
    c1 = Module.constants
    Object.module_eval "WALTER = 99"
    c2 = Module.constants
    assert_equal(["WALTER"], c2 - c1)
  end

  module M1
    $m1 = Module.nesting
    module M2
      $m2 = Module.nesting
    end
  end

  def test_s_nesting
    assert_equal([],                               $m0)
    assert_equal([TestModule::M1, TestModule],     $m1)
    assert_equal([TestModule::M1::M2,
                  TestModule::M1, TestModule],     $m2)
  end

  def test_s_new
    m = Module.new
    assert_instance_of(m, Module)
  end

end

Rubicon::handleTests(TestModule) if $0 == __FILE__
