$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestAssignment < Rubicon::TestCase

  def testExpansionWithNoConversion
    *x = (1..7).to_a
    assert_equal(7, x.size)
    assert_equal([1, 2, 3, 4, 5, 6, 7], x)

    assert_equal(7, x.size)
    assert_equal([1, 2, 3, 4, 5, 6, 7], x)
  end

  def testMultipleAssignment
    a, b = 1, 2
    assert_equal(1, a)
    assert_equal(2, b)
    
    a, b = b, a
    assert_equal(2, a)
    assert_equal(1, b)
    
    a, = 1,2
    assert_equal(1, a)
    
    a, *b = 1, 2, 3
    assert_equal(1, a)
    assert_equal([2, 3], b)
    
    a, (b, c), d = 1, [2, 3], 4
    assert_equal(1, a)
    assert_equal(2, b)
    assert_equal(3, c)
    assert_equal(4, d)
    
    *a = 1, 2, 3
    assert_equal([1, 2, 3], a)
    
    *a = 4
    assert_equal([4], a)
    
    *a = nil
    assert_equal([nil], a)
  end

  def testConditionalAssignment
    a=[]
    a[0] ||= "bar"
    assert_equal("bar", a[0])

    h={}
    h["foo"] ||= "bar"
    assert_equal("bar", h["foo"])
    
    aa = 5
    aa ||= 25
    assert_equal(5, aa)

    bb ||= 25
    assert_equal(25, bb)

    cc &&=33
    assert_nil(cc)

    cc = 5
    cc &&=44
    assert_equal(44, cc)
  end

end

# Run these tests if invoked directly

Rubicon::handleTests(TestAssignment) if $0 == __FILE__

