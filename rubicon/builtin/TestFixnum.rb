require '../rubicon'


class TestFixnum < Rubicon::TestCase

  # assumes 8 bit byte, 1 bit flag, and 2's comp
  MAX = 2**(1.size*8 - 2) - 1
  MIN = -MAX - 1

  def test_UMINUS
    [ -99, -1, 0, +1 , +99].each { |n| assert_equal(0, -n + n) }
  end

  def test_AND # '&'
    n1 = 0b0101
    n2 = 0b1100
    assert_equal(0,      n1 & 0)
    assert_equal(0b0100, n1 & n2)
    assert_equal(n1,     n1 & -1)
  end

  def test_AREF # '[]'
    checkBits([], 0)
    checkBits([15, 11, 7, 3], 0x8888)
    checkBits((0..(1.size*8-1)).to_a, -1)
  end

  def test_CMP # '<=>'
    assert_equal(0, MAX <=> MAX)
    assert_equal(0, MIN <=> MIN)
    assert_equal(0, 1   <=> 1)

    assert_equal(1, MAX <=> MIN)
    assert_equal(1, MAX <=> 0)
    assert_equal(1, MAX <=> 1)

    assert_equal(-1, MIN <=> MAX)
    assert_equal(-1, MIN <=> 0)
    assert_equal(-1, MIN <=> 1)
  end

  def test_DIV # '/'
    assert_equal(2, 6 / 3)
    assert_equal(2, 7 / 3)
    assert_equal(2, 8 / 3)
    assert_equal(3, 9 / 3)

    assert_equal(2, -6 / -3)
    assert_equal(2, -7 / -3)
    assert_equal(2, -8 / -3)
    assert_equal(3, -9 / -3)

    assert_equal(-2, -6 / 3)
    assert_equal(-2, -7 / 3)
    assert_equal(-2, -8 / 3)
    assert_equal(-3, -9 / 3)
    
    assert_equal(-2, 6 / -3)
    assert_equal(-2, 7 / -3)
    assert_equal(-2, 8 / -3)
    assert_equal(-3, 9 / -3)
    
    assert_equal(-1, MIN / MAX)
    assert_equal(0,  MAX / MIN)
  end

  def test_EQUAL # '=='
    assert(0 == 0)
    assert(MIN == MIN)
    assert(MAX == MAX)
    assert(!(MIN == MAX))
    assert(!(1 == 0))
  end

  def test_GE # '>='
    assert(0 >= 0)
    assert(1 >= 0)
    assert(MAX >= 0)
    assert(0 >= MIN)

    assert(!(0 >= 1))
    assert(!(0 >= MAX))
    assert(!(MIN >= 0))
  end

  def test_GT # '>'
    assert(1 > 0)
    assert(MAX > 0)
    assert(0 > MIN)

    assert(!(0 > 0))
    assert(!(0 > 1))
    assert(!(0 > MAX))
    assert(!(MIN > 0))
  end

  def test_LE # '<='
    assert(0 <= 0)
    assert(0 <= 1)
    assert(0 <= MAX)
    assert(MIN <= 0)

    assert(!(1 <= 0))
    assert(!(MAX <= 0))
    assert(!(0 <= MIN))
  end

  def test_LSHIFT # '<<'
    assert_equal(0, 0 << 1)
    assert_equal(2, 1 << 1)
    assert_equal(8, 1 << 3)
    assert_equal(2**20, 1 << 20)

    assert_equal(-2, (-1) << 1)
    assert_equal(-8, (-1) << 3)

    a = 1 << (1.size*8 - 2)
    assert_instance_of(a, Bignum)

    a = (-1) << (1.size*8 - 1)
    assert_instance_of(a, Bignum)
  end

  def test_LT # '<'
    assert(0 < 1)
    assert(0 < MAX)
    assert(MIN < 0)

    assert(!(0 < 0))
    assert(!(1 < 0))
    assert(!(MAX < 0))
    assert(!(0 < MIN))
  end

  def test_MINUS # '-'
    assert_fail("untested")
  end

  def test_MOD # '%'
    assert_fail("untested")
  end

  def test_MUL # '*'
    assert_fail("untested")
  end

  def test_OR # '|'
    n1 = 0b0101
    n2 = 0b1100
    assert_equal(n1,     n1 | 0)
    assert_equal(0b1101, n1 | n2)
    assert_equal(-1,     n1 | -1)
  end

  def test_PLUS # '+'
    assert_fail("untested")
  end

  def test_POW # '**'
    assert_fail("untested")
  end

  def test_REV # '~'
    n1 = -1
    n2 = 0b1100
    assert_equal(0, ~n1)
    assert_equal(-2, ~1)
    assert_equal(n2, ~(~n2))
  end

  def test_RSHIFT # '>>'
    assert_fail("untested")
  end

  def test_XOR # '^'
    n1 = 0b0101
    n2 = 0b1100
    assert_equal(0b1001, n1 ^ n2)
    assert_equal(n1,     n1 ^ 0)
    assert_equal(~n1,    n1 ^ -1)
  end

  def test_abs
    assert_fail("untested")
  end

  def test_downto
    assert_fail("untested")
  end

  def test_id2name
    assert_fail("untested")
  end

  def test_next
    assert_fail("untested")
  end

  def test_remainder
    assert_fail("untested")
  end

  def test_size
    assert_fail("untested")
  end

  def test_step
    assert_fail("untested")
  end

  def test_succ
    assert_fail("untested")
  end

  def test_times
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

  def test_type
    assert_fail("untested")
  end

  def test_upto
    assert_fail("untested")
  end

  def test_zero?
    assert_fail("untested")
  end

  def test_s_induced_from
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestFixnum) if $0 == __FILE__
