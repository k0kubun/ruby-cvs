require '../rubicon'


class TestHash < Rubicon::TestCase

  def setup
    @h = {
      1 => 'one', 2 => 'two', 3 => 'three',
      self => 'self', true => 'true', nil => 'nil',
      'nil' => nil
    }
  end

  def test_s_aref
    h = Hash["a" => 100, "b" => 200]
    assert_equal(100, h['a'])
    assert_equal(200, h['b'])
    assert_nil(h['c'])

    h = Hash.[]("a" => 100, "b" => 200)
    assert_equal(100, h['a'])
    assert_equal(200, h['b'])
    assert_nil(h['c'])
  end

  def test_s_new
    h = Hash.new
    assert_instance_of(h, Hash)
    assert_nil(h.default)
    assert_nil(h['spurious'])

    h = Hash.new('default')
    assert_instance_of(h, Hash)
    assert_equal('default', h.default)
    assert_equal('default', h['spurious'])
    
  end

  def test_AREF # '[]'
    t = Time.now
    h = {
      1 => 'one', 2 => 'two', 3 => 'three',
      self => 'self', t => 'time', nil => 'nil',
      'nil' => nil
    }

    assert_equal('one',   h[1])
    assert_equal('two',   h[2])
    assert_equal('three', h[3])
    assert_equal('self',  h[self])
    assert_equal('time',  h[t])
    assert_equal('nil',   h[nil])
    assert_equal(nil,     h['nil'])
    assert_equal(nil,     h['koala'])

    h1 = h.dup
    h1.default = :default

    assert_equal('one',    h1[1])
    assert_equal('two',    h1[2])
    assert_equal('three',  h1[3])
    assert_equal('self',   h1[self])
    assert_equal('time',   h1[t])
    assert_equal('nil',    h1[nil])
    assert_equal(nil,      h1['nil'])
    assert_equal(:default, h1['koala'])


  end

  def test_ASET # '[]='
    t = Time.now
    h = Hash.new
    h[1]     = 'one'
    h[2]     = 'two'
    h[3]     = 'three'
    h[self]  = 'self'
    h[t]     = 'time'
    h[nil]   = 'nil'
    h['nil'] = nil
    assert_equal('one',   h[1])
    assert_equal('two',   h[2])
    assert_equal('three', h[3])
    assert_equal('self',  h[self])
    assert_equal('time',  h[t])
    assert_equal('nil',   h[nil])
    assert_equal(nil,     h['nil'])
    assert_equal(nil,     h['koala'])

    h[1] = 1
    h[nil] = 99
    h['nil'] = nil
    assert_equal(1,       h[1])
    assert_equal('two',   h[2])
    assert_equal('three', h[3])
    assert_equal('self',  h[self])
    assert_equal('time',  h[t])
    assert_equal(99,      h[nil])
    assert_equal(nil,     h['nil'])
    assert_equal(nil,     h['koala'])
  end

  def test_EQUAL # '=='
    h1 = { "a" => 1, "c" => 2 } 
    h2 = { "a" => 1, "c" => 2, 7 => 35 }
    h3 = { "a" => 1, "c" => 2, 7 => 35 }
    h4 = { }
    assert(h1 == h1)
    assert(h2 == h2)
    assert(h3 == h3)
    assert(h4 == h4)
    assert(!(h1 == h2))
    assert(h2 == h3)
    assert(!(h3 == h4))
  end

  def test_clear
    assert(@h.size > 0)
    @h.clear
    assert_equal(0, @h.size)
    assert_nil(@h[1])
  end

  def test_clone
    h = @h.clone
    assert(@h == h)
    assert(!@h.tainted?)
    assert(!h.tainted?)
    assert(!@h.frozen?)
    assert(!h.frozen?)

    h.freeze
    h1 = h.clone
    assert(h1 == h)
    assert(!h1.tainted?)
    assert(!h.tainted?)
    assert(h1.frozen?)
    assert(h.frozen?)

    h.taint
    h1 = h.clone
    assert(h1 == h)
    assert(h1.tainted?)
    assert(h.tainted?)
    assert(h1.frozen?)
    assert(h.frozen?)
  end

  def test_default
    assert_nil(@h.default)
    h = Hash.new(:xyzzy)
    assert_equal(:xyzzy, h.default)
  end

  def test_default=
    assert_nil(@h.default)
    @h.default = :xyzzy
    assert_equal(:xyzzy, @h.default)
  end

  def test_delete
    h1 = { 1 => 'one', 2 => 'two', true => 'true' }
    h2 = { 1 => 'one', 2 => 'two' }
    h3 = { 2 => 'two' }
    
    assert_equal('true', h1.delete(true))
    assert_equal(h2, h1)

    assert_equal('one', h1.delete(1))
    assert_equal(h3, h1)

    assert_equal('two', h1.delete(2))
    assert_equal({}, h1)

    assert_nil(h1.delete(99))
    assert_equal({}, h1)

    assert_equal('default 99', h1.delete(99) {|i| "default #{i}" })
  end

  def test_delete_if
    base = { 1 => 'one', 2 => false, true => 'true', 'cat' => 99 }
    h1   = { 1 => 'one', 2 => false, true => 'true' }
    h2   = { 2 => false, 'cat' => 99 }
    h3   = { 2 => false }

    h = base.dup
    assert_equal(h, h.delete_if { false })
    assert_equal({}, h.delete_if { true })

    h = base.dup
    assert_equal(h1, h.delete_if {|k,v| k.instance_of?(String) })

    h = base.dup
    assert_equal(h2, h.delete_if {|k,v| v.instance_of?(String) })

    h = base.dup
    assert_equal(h3, h.delete_if {|k,v| v })
  end

  def test_dup
    h = @h.dup
    assert(@h == h)
    assert(!@h.tainted?)
    assert(!h.tainted?)
    assert(!@h.frozen?)
    assert(!h.frozen?)

    h.freeze
    h1 = h.dup
    assert(h1 == h)
    assert(!h1.tainted?)
    assert(!h.tainted?)
    assert(!h1.frozen?)
    assert(h.frozen?)

    h.taint
    h1 = h.dup
    assert(h1 == h)
    assert(h1.tainted?)
    assert(h.tainted?)
    assert(!h1.frozen?)
    assert(h.frozen?)
  end

  def test_each
    assert_fail("untested")
  end

  def test_each_key
    assert_fail("untested")
  end

  def test_each_pair
    assert_fail("untested")
  end

  def test_each_value
    assert_fail("untested")
  end

  def test_empty?
    assert_fail("untested")
  end

  def test_fetch
    assert_fail("untested")
  end

  def test_has_key?
    assert_fail("untested")
  end

  def test_has_value?
    assert_fail("untested")
  end

  def test_include?
    assert_fail("untested")
  end

  def test_index
    assert_fail("untested")
  end

  def test_indexes
    assert_fail("untested")
  end

  def test_indices
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_invert
    assert_fail("untested")
  end

  def test_key?
    assert_fail("untested")
  end

  def test_keys
    assert_fail("untested")
  end

  def test_length
    assert_fail("untested")
  end

  def test_member?
    assert_fail("untested")
  end

  def test_rehash
    assert_fail("untested")
  end

  def test_reject
    assert_fail("untested")
  end

  def test_reject!
    assert_fail("untested")
  end

  def test_replace
    assert_fail("untested")
  end

  def test_shift
    assert_fail("untested")
  end

  def test_size
    assert_fail("untested")
  end

  def test_sort
    assert_fail("untested")
  end

  def test_store
    assert_fail("untested")
  end

  def test_to_a
    assert_fail("untested")
  end

  def test_to_hash
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_update
    assert_fail("untested")
  end

  def test_value?
    assert_fail("untested")
  end

  def test_values
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestHash) if $0 == __FILE__
