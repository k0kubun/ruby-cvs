$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestRegularExpressions < Rubicon::TestCase

  def testBasics
    assert("abc" !~ /^$/)
    assert("abc\n" !~ /^$/)
    assert("abc" !~ /^d*$/)
    assert(("abc" =~ /d*$/) == 3)
    assert("" =~ /^$/)
    assert("\n" =~ /^$/)
    assert("a\n\n" =~ /^$/)
    assert("abcabc" =~ /.*a/ && $& == "abca")
    assert("abcabc" =~ /.*c/ && $& == "abcabc")
    assert("abcabc" =~ /.*?a/ && $& == "a")
    assert("abcabc" =~ /.*?c/ && $& == "abc")
    assert(/(.|\n)*?\n(b|\n)/ =~ "a\nb\n\n" && $& == "a\nb")
    
    assert(/^(ab+)+b/ =~ "ababb" && $& == "ababb")
    assert(/^(?:ab+)+b/ =~ "ababb" && $& == "ababb")
    assert(/^(ab+)+/ =~ "ababb" && $& == "ababb")
    assert(/^(?:ab+)+/ =~ "ababb" && $& == "ababb")
    
    assert(/(\s+\d+){2}/ =~ " 1 2" && $& == " 1 2")
    assert(/(?:\s+\d+){2}/ =~ " 1 2" && $& == " 1 2")

    x = "ABCD\nABCD\n"
    x.gsub!(/((.|\n)*?)B((.|\n)*?)D/) {$1+$3}
    assert_equal("AC\nAC\n", x)

    assert("foobar" =~ /foo(?=(bar)|(baz))/)
    assert("foobaz" =~ /foo(?=(bar)|(baz))/)
  end

  def testReferences
    x = "a.gif"
    assert_equal("gif",     x.sub(/.*\.([^\.]+)/, '\1'))
    assert_equal("b.gif",   x.sub(/.*\.([^\.]+)/, 'b.\1'))
    assert_equal("",        x.sub(/.*\.([^\.]+)/, '\2'))
    assert_equal("ab",      x.sub(/.*\.([^\.]+)/, 'a\2b'))
    assert_equal("<a.gif>", x.sub(/.*\.([^\.]+)/, '<\&>'))
  end

end

# Run these tests if invoked directly

Rubicon::handleTests(TestRegularExpressions) if $0 == __FILE__
