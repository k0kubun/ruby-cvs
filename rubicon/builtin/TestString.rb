require '../rubicon'


class TestString < Rubicon::TestCase

  def test_AREF # '[]'
    assert_fail("untested")
  end

  def test_ASET # '[]='
    assert_fail("untested")
  end

  def test_CMP # '<=>'
    assert_fail("Remember $=")
  end

  def test_EQUAL # '=='

    $= = true
    assert("CAT" == 'cat')
    assert("CaT" == 'cAt')
    $= = false
    assert_fail("untested")
  end

  def test_LSHIFT # '<<'
    assert_fail("untested")
  end

  def test_MATCH # '=~'
    assert_fail("Remember $=")
    assert_fail("untested")
  end

  def test_MOD # '%'
    assert_fail("untested")
  end

  def test_MUL # '*'
    assert_fail("untested")
  end

  def test_PLUS # '+'
    assert_fail("untested")
  end

  def test_REV # '~'
    assert_fail("untested")
  end

  def test_VERY_EQUAL # '==='
    assert_fail("Remember $=")
    assert_fail("untested")
  end

  def test_capitalize
    assert_fail("untested")
  end

  def test_capitalize!
    assert_fail("untested")
  end

  def test_center
    assert_fail("untested")
  end

  def test_chomp
    assert_fail("untested")
  end

  def test_chomp!
    assert_fail("untested")
  end

  def test_chop
    assert_fail("untested")
  end

  def test_chop!
    assert_fail("untested")
  end

  def test_clone
    assert_fail("untested")
  end

  def test_concat
    assert_fail("untested")
  end

  def test_count
    assert_fail("untested")
  end

  def test_crypt
    assert_fail("untested")
  end

  def test_delete
    assert_fail("untested")
  end

  def test_delete!
    assert_fail("untested")
  end

  def test_downcase
    assert_fail("untested")
  end

  def test_downcase!
    assert_fail("untested")
  end

  def test_dump
    assert_fail("untested")
  end

  def test_dup
    assert_fail("untested")
  end

  def test_each
    assert_fail("untested")
  end

  def test_each_byte
    assert_fail("untested")
  end

  def test_each_line
    assert_fail("untested")
  end

  def test_empty?
    assert_fail("untested")
  end

  def test_eql?
    assert_fail("untested")
  end

  def test_gsub
    assert_fail("untested")
  end

  def test_gsub!
    assert_fail("untested")
  end

  def test_hash
    assert_fail("untested")
  end

  def test_hex
    assert_fail("untested")
  end

  def test_include?
    assert_fail("untested")
  end

  def test_index
    assert_fail("untested")
  end

  def test_inspect
    assert_fail("untested")
  end

  def test_intern
    assert_fail("untested")
  end

  def test_length
    assert_fail("untested")
  end

  def test_ljust
    assert_fail("untested")
  end

  def test_next
    assert_fail("untested")
  end

  def test_next!
    assert_fail("untested")
  end

  def test_oct
    assert_fail("untested")
  end

  def test_replace
    assert_fail("untested")
  end

  def test_reverse
    assert_fail("untested")
  end

  def test_reverse!
    assert_fail("untested")
  end

  def test_rindex
    assert_fail("untested")
  end

  def test_rjust
    assert_fail("untested")
  end

  def test_scan
    assert_fail("untested")
  end

  def test_size
    assert_fail("untested")
  end

  def test_slice
    assert_fail("untested")
  end

  def test_slice!
    assert_fail("untested")
  end

  def test_split
    assert_fail("untested")
  end

  def test_squeeze
    assert_fail("untested")
  end

  def test_squeeze!
    assert_fail("untested")
  end

  def test_strip
    assert_fail("untested")
  end

  def test_strip!
    assert_fail("untested")
  end

  def test_sub
    assert_fail("untested")
  end

  def test_sub!
    assert_fail("untested")
  end

  def test_succ
    assert_fail("untested")
  end

  def test_succ!
    assert_fail("untested")
  end

  def test_sum
    assert_fail("untested")
  end

  def test_swapcase
    assert_fail("untested")
  end

  def test_swapcase!
    assert_fail("untested")
  end

  def test_to_f
    assert_fail("untested")
  end

  def test_to_i
    assert_fail("untested")
  end

  def test_to_s
    assert_fail("untested")
  end

  def test_to_str
    assert_fail("untested")
  end

  def test_tr
    assert_fail("untested")
  end

  def test_tr!
    assert_fail("untested")
  end

  def test_tr_s
    assert_fail("untested")
  end

  def test_tr_s!
    assert_fail("untested")
  end

  def test_unpack
    assert_fail("untested")
  end

  def test_upcase
    assert_fail("untested")
  end

  def test_upcase!
    assert_fail("untested")
  end

  def test_upto
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestString) if $0 == __FILE__
