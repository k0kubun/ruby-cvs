require '../rubicon'


class TestMarshal < Rubicon::TestCase

  class A
    attr :a1
    attr :a2
    def initialize(a1, a2)
      @a1, @a2 = a1, a2
    end
  end

  class B
    attr :b1
    attr :b2
    def initialize(b1, b2)
      @b1 = A.new(b1, 2*b1)
      @b2 = b2
    end
  end

  # Dump/load to string
  def test_s_dump_load1
    b = B.new(10, "wombat")
    assert_equal(10,       b.b1.a1)
    assert_equal(20,       b.b1.a2)
    assert_equal("wombat", b.b2)

    s = Marshal.dump(b)

    assert_instance_of(String, s)

    newb = Marshal.load(s)
    assert_equal(10,       newb.b1.a1)
    assert_equal(20,       newb.b1.a2)
    assert_equal("wombat", newb.b2)

    assert(newb.id != b.id)

    assert_exception(ArgumentError) { Marshal.dump(b, 1) }
  end

  def test_s_dump_load2
    b = B.new(10, "wombat")
    assert_equal(10,       b.b1.a1)
    assert_equal(20,       b.b1.a2)
    assert_equal("wombat", b.b2)

    File.open("_dl", "w") { |f| Marshal.dump(b, f) }
    
    begin
      newb = nil
      File.open("_dl") { |f| newb = Marshal.load(f) }

      assert_equal(10,       newb.b1.a1)
      assert_equal(20,       newb.b1.a2)
      assert_equal("wombat", newb.b2)
      
    ensure
      File.delete("_dl")
    end
  end

  def test_s_dump_load3
    b = B.new(10, "wombat")
    s = Marshal.dump(b)

    res = []
    newb = Marshal.load(s, proc { |obj| res << obj })

    assert_equal(10,       newb.b1.a1)
    assert_equal(20,       newb.b1.a2)
    assert_equal("wombat", newb.b2)

    assert_set_equal([newb, newb.b1, newb.b2], res)
  end

  def test_s_restore
    b = B.new(10, "wombat")
    assert_equal(10,       b.b1.a1)
    assert_equal(20,       b.b1.a2)
    assert_equal("wombat", b.b2)

    s = Marshal.dump(b)

    assert_instance_of(String, s)

    newb = Marshal.restore(s)
    assert_equal(10,       newb.b1.a1)
    assert_equal(20,       newb.b1.a2)
    assert_equal("wombat", newb.b2)

    assert(newb.id != b.id)
  end

end

Rubicon::handleTests(TestMarshal) if $0 == __FILE__
