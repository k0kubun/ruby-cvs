$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

# Tests of functions in the String class are mostly in ../builtins/TestString.rb

class TestStrings < Rubicon::TestCase

  def testCompileTimeStringConcatenation
    assert_equal("abcd", "ab" "cd")
    assert_equal("abcd", 'ab' "cd")
    assert_equal("abcd", 'ab' 'cd')
    assert_equal("abcd", "ab" 'cd')
    assert_equal("22aacd44", "#{22}aa" "cd#{44}")
    assert_equal("22aacd445566", "#{22}aa" "cd#{44}" "55" "#{66}")
  end

  # ------------------------------------------------------------

  def testInterpolationOfGlobal
    $foo = "abc"
    assert_equal("abc = abc", "#$foo = abc")
    assert_equal("abc = abc", "#{$foo} = abc")
    
    foo = "abc"
    assert_equal("abc = abc", "#{foo} = abc")
  end

  # ------------------------------------------------------------

  # ------------------------------------------------------------


end

# Run these tests if invoked directly

Rubicon::handleTests(TestStrings) if $0 == __FILE__
