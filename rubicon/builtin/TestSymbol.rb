require '../rubicon'

class Fred
  $f1 = :Fred
end

module Test
  Fred = 1
  $f2 = :Fred
end

def Fred
$f3 = :Fred
end


class TestSymbol < Rubicon::TestCase

  def test_00sanity
    assert_equals($f1.id,$f2.id)
    assert_equals($f2.id,$f3.id)
  end

  def test_id2name
    assert_equals("Fred",:Fred.id2name)
    assert_equals("Barney",:Barney.id2name)
    assert_equals("wilma",:wilma.id2name)
  end

  def test_inspect
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

end

Rubicon::handleTests(TestSymbol) if $0 == __FILE__
