$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

#
# This is the test of all the private methods of Module that can really
# only be tested indirectly
#

unless defined? ExpectedException
  Version.less_than("1.7") do
    ExpectedException = NameError
  end
  Version.greater_or_equal("1.7") do
    ExpectedException = NoMethodError
  end 
end


class TestModulePrivate < Rubicon::TestCase

  class T1
    def m1
      "m1"
    end
    alias_method :new_m1, :m1
  end

  def test_alias_method
    t1 = T1.new
    assert_equal("m1", t1.m1)
    assert_equal("m1", t1.new_m1)
    assert_set_equal(["new_m1", "m1"], T1.instance_methods)
  end

  # --------------------------------------------------------

  def test_append_features
    skipping("How do I test")
  end

  # --------------------------------------------------------

  class C3
    attr :a
    attr :b, true
    def initialize
      @a = 1
      @b = 2
    end
  end

  def test_attr
    assert_set_equal(["a", "b", "b="], C3.instance_methods)
    c3 = C3.new
    assert_equal(1, c3.a)
    assert_equal(2, c3.b)
    c3.b = "cat"
    assert_equal("cat", c3.b)
  end

  # --------------------------------------------------------

  class C4
    attr_accessor :a, :b
    def initialize
      @a = 1
      @b = 2
    end
  end

  def test_attr_accessor
    assert_set_equal(["a", "a=", "b", "b="], C4.instance_methods)
    c4 = C4.new
    assert_equal(1, c4.a)
    assert_equal(2, c4.b)
    c4.a = "dog"
    assert_equal("dog", c4.a)
    c4.b = "cat"
    assert_equal("cat", c4.b)
  end

  # --------------------------------------------------------

  class C5
    attr_reader :a, :b
    def initialize
      @a = 1
      @b = 2
    end
  end

  def test_attr_reader
    assert_set_equal(["a", "b"], C5.instance_methods)
    c5 = C5.new
    assert_equal(1, c5.a)
    assert_equal(2, c5.b)
  end

  # --------------------------------------------------------

  class C6
    attr_writer :a, :b
    def initialize
      @a = 1
      @b = 2
    end
    def getem
      [ @a, @b ]
    end
  end

  def test_attr_writer
    assert_set_equal(["a=", "b=", "getem"], C6.instance_methods)
    c6 = C6.new
    assert_equal([1, 2], c6.getem)
    c6.a = "cat"
    c6.b = "dog"
    assert_equal(["cat", "dog"], c6.getem)
  end

  # --------------------------------------------------------

  module M7A
    def m7
      "m7"
    end
  end

  module M7
    include M7A
  end

  class C7
    include M7
  end
  
  def test_include
    assert_set_equal([],                M7A.included_modules)
    assert_set_equal([M7A],             M7.included_modules)
    assert_set_equal([M7A, M7, Kernel], C7.included_modules)

    assert_equal("m7", C7.new.m7)
  end

  # --------------------------------------------------------

  class C8
    M = []
    def C8.method_added(id)
      M << id
    end
    def C8.meths
      M
    end
    def one()   end
  end
  class C8
    def two()   end
  end

  def test_method_added
    assert_set_equal([:one, :two], C8.meths)
    C8.class_eval "def three() end"
    assert_set_equal([:one, :two, :three], C8.meths)
  end

  # --------------------------------------------------------
  
  module M9
    def m9
      "m9"
    end
    module_function :m9
  end
  
  class C9
    include M9
    def call_m9
      m9
    end
  end

  def test_module_function
    assert_equal("m9", M9.m9)
    assert_equal("m9", C9.new.call_m9)
  end

  # --------------------------------------------------------
  
  class C10A
    def one() end
    private
    def two() end
    protected
    def three() end
    public
    def four() end
  end

  class C10B
    def one() end
    def two() end
    def three() end
    def four() end
    private :two
    protected :three
    public :four
  end

  def test_private
    assert_equal(["two"], C10A.private_instance_methods)
    assert_equal(["two"], C10B.private_instance_methods)
  end

  # --------------------------------------------------------

  def test_protected
    assert_equal(["three"], C10A.protected_instance_methods)
    assert_equal(["three"], C10B.protected_instance_methods)
  end

  # --------------------------------------------------------

  def test_public
    assert_set_equal(["one", "four"], C10A.public_instance_methods)
    assert_set_equal(["one", "four"], C10B.public_instance_methods)
  end

  # --------------------------------------------------------

  module M13
    ONE = 1
    TWO = 2
    def M13.hackOne
      remove_const :ONE
    end
    def M13.hackTwo
      remove_const :TWO
    end
  end

  def test_remove_const
    assert_set_equal(["ONE", "TWO"], M13.constants)
    M13.hackOne
    assert_set_equal(["TWO"], M13.constants)
    M13.hackTwo
    assert_set_equal([], M13.constants)
  end

  # --------------------------------------------------------

  class C14A
    def c14
      "c14a"
    end
  end
  class C14 < C14A
    def c14
      "c14"
    end
  end
  
  def test_remove_method
    c14 = C14.new
    assert_equal("c14", c14.c14)
    C14.class_eval "remove_method :c14"
    assert_equal("c14a", c14.c14)
    C14A.class_eval "remove_method :c14"
    assert_exception(ExpectedException) { eval "c14.c14" }
  end

  # --------------------------------------------------------

  class C15A
    def c15
      "c15a"
    end
  end
  class C15 < C15A
    def c15
      "c15"
    end
  end
  
  def test_undef_method
    c15 = C15.new
    assert_equal("c15", c15.c15)
    C15.class_eval "remove_method :c15"
    assert_equal("c15a", c15.c15)
    C15.class_eval "undef_method :c15"
    assert_exception(ExpectedException) { eval "c15.c15" }
  end

end


Rubicon::handleTests(TestModulePrivate) if $0 == __FILE__
