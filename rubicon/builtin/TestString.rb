require '../rubicon'

class TestString < Rubicon::TestCase

  def test_AREF # '[]'
    assert_equal(65,"AooBar"[0])
    assert_equal(66,"FooBaB"[-1])
    assert_equal(nil,"FooBar"[6])
    assert_equal(nil,"FooBar"[-7])

    assert_equal("Foo","FooBar"[0,3])
    assert_equal("Bar","FooBar"[-3,3])
    $stderr.puts "End of string is special???"
    assert_equal(nil,"FooBar"[7,2])     # Maybe should be six?
    assert_equal(nil,"FooBar"[-7,10])

    assert_equal("Foo","FooBar"[0..2])
    assert_equal("Bar","FooBar"[-3..-1])
    assert_equal(nil,"FooBar"[6..2])
    assert_equal(nil,"FooBar"[-10..-7])

    assert_equal("Foo","FooBar"[/^F../])
    assert_equal("Bar","FooBar"[/..r$/])
    assert_equal(nil,"FooBar"[/xyzzy/])
    assert_equal(nil,"FooBar"[/plugh/])

    assert_equal("Foo","FooBar"["Foo"])
    assert_equal("Bar","FooBar"["Bar"])
    assert_equal(nil,"FooBar"["xyzzy"])
    assert_equal(nil,"FooBar"["plugh"])
  end

  def test_ASET # '[]='
    s = "FooBar"
    s[0] = 'A'
    assert_equal("AooBar",s)
    s[-1]= 'B'
    assert_equal("AooBaB",s)
    assert_exception(IndexError) { s[-7] = "xyz" }
    assert_equal("AooBaB",s)
    s[0] = "ABC"
    assert_equal("ABCooBaB",s)

    s="FooBar"
    s[0,3] = "A"
    assert_equal("ABar",s)
    s[0] = "Foo"
    assert_equal("FooBar",s)
    s[-3,3] = "Foo"
    assert_equal("FooFoo",s)
    assert_exception (IndexError) { s[7,3] = "Bar" }
    assert_exception (IndexError) { s[-7,3] = "Bar" }

    s="FooBar"
    s[0..2] = "A"
    assert_equal("ABar",s)
    s[1..3] = "Foo"
    assert_equal("AFoo",s)
    s[-4..-4] = "Foo"
    assert_equal("FooFoo",s)
    assert_exception (RangeError) { s[7..10] = "Bar" }
    assert_exception (RangeError) { s[-7..-10] = "Bar" }


    s="FooBar"
    s[/^F../]= "Bar"
    assert_equal("BarBar",s)
    s[/..r$/]="Foo"
    assert_equal("BarFoo",s)
    s[/xyzzy/] = "None"
    assert_equal("BarFoo",s)

    s="FooBar"
    s["Foo"] = "Bar"
    assert_equal("BarBar",s)
    s["Foo"] = "xyz"
    assert_equal("BarBar",s)

    $= = true
    s="FooBar"
    s["FOO"] = "Bar"
    assert_equal("BarBar",s)
    s["FOO"] = "xyz"
    assert_equal("BarBar",s)
    $= = false
  end

  def test_CMP # '<=>'
    assert(1, "abcdef" <=> "abcde")
    assert(0, "abcdef" <=> "abcdef")
    assert(-1, "abcde" <=> "abcdef")

    assert(1, "ABCDEF" <=> "abcdef")
    $= = true
    assert(0, "ABCDEF" <=> "abcdef")
    $= = false
  end

  def test_EQUAL # '=='
    assert_equal(false, "foo" == :foo)
    assert(0, "abcdef" == "abcdef")
    $= = true
    assert("CAT" == 'cat')
    assert("CaT" == 'cAt')
    $= = false
    assert("CAT" != 'cat')
    assert("CaT" != 'cAt')
  end

  def test_LSHIFT # '<<'
    assert_equal("world!", "world" << 33)
    assert_equal("world!", "world" << '!')
  end

  def test_MATCH # '=~'
    assert_equal(10, "FeeFieFoo-Fum" =~ /Fum$/)
    assert_equal(nil, "FeeFieFoo-Fum" =~ /FUM$/)
    $= = true
    assert_equal(10, "FeeFieFoo-Fum" =~ /FUM$/)
    $= = false
  end

  def test_MOD # '%'
    assert_equals("00123", "%05d" % 123)
    assert_equals("123  |00000001", "%-5s|%08x" % [123, 1])
    $stderr.puts "kernel.sprintf flags not tested in String.%"
  end

  def test_MUL # '*'
    assert_equals("XXX","X" * 3)
    assert_equals("HOHO","HO" * 2)
  end

  def test_PLUS # '+'
    assert_equals("Yodel", "Yo" + "del")
  end

  def test_REV # '~'
    $_ = "FeeFieFoo-Fum"
    assert_equal(10, ~"Fum$")
    assert_equal(nil, ~"FUM$")
    $= = true
    assert_equal(10, ~"FUM$")
    $= = false
    
  end

  def casetest(a,b,rev=false)
    case a
      when b
        assert_fail("=== failed") if rev
        assert(true)
      else
        assert_fail("=== failed") unless rev
        assert(true)
    end
  end

  def test_VERY_EQUAL # '==='
    assert_equal(false, "foo" === :foo)
    casetest("abcdef","abcdef")
    $= = true
    casetest("CAT",'cat')
    casetest("CaT",'cAt')
    $= = false
    casetest("CAT",'cat',true) # Reverse the test - we don't want to
    casetest("CaT",'cAt',true) # find these in the case.
  end

  def test_capitalize
    assert_equal("Hello", "hello".capitalize)
    assert_equal("Hello", "hELLO".capitalize)
    assert_equal("123abc", "123ABC".capitalize)
  end

  def test_capitalize!
    a="hello"; a.capitalize!
    assert_equal("Hello", a)
    a="hELLO"; a.capitalize!
    assert_equal("Hello", a)
    a="123ABC"; a.capitalize!
    assert_equal("123abc", a)
    assert_equal(nil,"123abc".capitalize!)
    assert_equal("123abc","123ABC".capitalize!)
  end

  def test_center
    assert_equal("hello", "hello".center(4))
    assert_equal("   hello   ", "hello".center(11))
  end

  def test_chomp
    assert_equal("hello", "hello".chomp("\n"))
    assert_equal("hello", "hello\n".chomp("\n"))
    $/="\n"
    assert_equal("hello", "hello".chomp)
    assert_equal("hello", "hello\n".chomp)
    $/="!"
    assert_equal("hello", "hello".chomp)
    assert_equal("hello", "hello!".chomp)
    $/="\n"
  end

  def test_chomp!
    a="hello"
    a.chomp!("\n")
    assert_equal("hello", a)
    assert_equal(nil,a.chomp!("\n"))
    a="hello\n"
    a.chomp!("\n")
    assert_equal("hello", a)

    $/="\n"
    a="hello"
    a.chomp!
    assert_equal("hello", a)
    a="hello\n"
    a.chomp!
    assert_equal("hello", a)

    $/="!"
    a="hello"
    a.chomp!
    assert_equal("hello", a)
    a="hello!"
    a.chomp!
    assert_equal("hello", a)

    $/="\n"
  end

  def test_chop
    assert_equal("hell", "hello".chop)
    assert_equal("hello", "hello\r\n".chop)
    assert_equal("hello\n", "hello\n\r".chop)
    assert_equal("", "\r\n".chop)
    assert_equal("", "".chop)
  end

  def test_chop!
    a="hello".chop!
    assert_equal("hell", a)
    a="hello\r\n".chop!
    assert_equal("hello", a)
    a="hello\n\r".chop!
    assert_equal("hello\n", a)
    a="\r\n".chop!
    assert_equal("", a)
    a="".chop!
    assert_equal(nil,a)
  end

  def test_clone
    for taint in [ false, true ]
      for frozen in [ false, true ]
        a = "Cool"
        a.freeze if frozen
        a.taint  if taint
        b = a.clone

        assert_equal(a, b)
        assert(a.id != b.id)
        assert_equal(a.frozen?, b.frozen?)
        assert_equal(a.tainted?, b.tainted?)
      end
    end
  end

  def test_concat
    assert_equal("world!", "world".concat(33))
    assert_equal("world!", "world".concat('!'))
  end

  def test_count
    a = "hello world"
    assert_equal(5,a.count("lo"))
    assert_equal(2,a.count("lo","o"))
    assert_equal(4,a.count("hello","^l"))
    assert_equal(4,a.count("ej-m"))
  end

  def test_crypt
    assert_equal('aaGUC/JkO9/Sc',"mypassword".crypt("aa"))
    assert('aaGUC/JkO9/Sc' != "mypassword".crypt("ab"))
  end

  def test_delete
    assert_equal("heo","hello".delete("l","lo"))
    assert_equal("he","hello".delete("lo"))
    assert_equal("hell","hello".delete("aeiou","^e"))
    assert_equal("ho","hello".delete("ej-m"))
  end

  def test_delete!
    a="hello"
    a.delete!("l","lo")
    assert_equal("heo",a)
    a="hello"
    a.delete!("lo")
    assert_equal("he",a)

    a="hello"
    a.delete!("aeiou","^e")
    assert_equal("hell",a)

    a="hello"
    a.delete!("ej-m")
    assert_equal("ho",a)

    a="hello"
    assert_equal(nil,a.delete!("z"))
  end


  def test_downcase
    assert_equal("hello","hElLo".downcase)
    assert_equal("hello","hello".downcase)
    assert_equal("hello","HELLO".downcase)
    assert_equal("abc hello 123","abc HELLO 123".downcase)
  end

  def test_downcase!
    a="heLlO"
    a.downcase!
    assert_equal("hello",a)

    a="hello"
    assert_equal(nil,a.downcase!)
    assert_equal("hello",a)
  end

  def test_dump
    a= "Test" << 1 << 2 << 3 << 9 << 13 << 10
    assert_equal("Test\001\002\003\t\r\n",a)
  end

  def test_dup
    for taint in [ false, true ]
      for frozen in [ false, true ]
        a = "Hello"
        a.freeze if frozen
        a.taint  if taint
        b = a.dup 

        assert_equal(a, b)
        assert(a.id != b.id)
        assert_equal(false, b.frozen?)
        assert_equal(a.tainted?,b.tainted?)
      end
    end     
  end

  def test_each
    $/ = "\n"
    res=[]
    "hello\nworld".each {|x| res << x}
    assert_equal("hello\n",res[0])
    assert_equal("world",res[1])

    res=[]
    "hello\n\n\nworld".each('') {|x| res << x}
    assert_equal("hello\n\n\n",res[0])
    assert_equal("world",res[1])

    $/ = "!"
    res=[]
    "hello!world".each {|x| res << x}
    assert_equal("hello!",res[0])
    assert_equal("world",res[1])

    $/ = "\n"
  end

  def test_each_byte
    res = []
    "ABC".each_byte {|x| res << x }
    assert_equal(65, res[0])
    assert_equal(66, res[1])
    assert_equal(67, res[2])
  end

  def test_each_line
    $/ = "\n"
    res=[]
    "hello\nworld".each {|x| res << x}
    assert_equal("hello\n",res[0])
    assert_equal("world",res[1])

    res=[]
    "hello\n\n\nworld".each('') {|x| res << x}
    assert_equal("hello\n\n\n",res[0])
    assert_equal("world",res[1])

    $/ = "!"
    res=[]
    "hello!world".each {|x| res << x}
    assert_equal("hello!",res[0])
    assert_equal("world",res[1])

    $/ = "\n"
  end

  def test_empty?
    assert_equal(true, "".empty?)
    assert_equal(false, "not".empty?)
  end

  def test_eql?
    a = "Hello"
    assert_equal(true,a.eql?("Hello"))
    assert_equal(true,a.eql?(a))
  end

  def test_gsub
    assert_equal("h*ll*","hello".gsub(/[aeiou]/,'*'))
    assert_equal("h<e>ll<o>","hello".gsub(/([aeiou])/,'<\1>'))
    assert_equal("104 101 108 108 111 ", "hello".gsub('.') {
                   |s| s[0].to_s+' '})
    assert_equal("HELL-o", "hello".gsub(/(hell)(.)/) {
                   |s| $1.upcase + '-' + $2
                   })
    a = "hello"
    a.taint
    assert_equal(true, (a.gsub('.','X').tainted?))
  end

  def test_gsub!
    a = "hello"
    a.gsub!(/[aeiou]/,'*')
    assert_equal("h*ll*",a)

    a = "hello"
    a.gsub!(/([aeiou])/,'<\1>')
    assert_equal("h<e>ll<o>",a)

    a = "hello"
    a.gsub!('.') { |s| s[0].to_s+' '}
    assert_equal("104 101 108 108 111 ", a)

    a = "hello"
    a.gsub!(/(hell)(.)/) { |s| $1.upcase + '-' + $2 }
    assert_equal("HELL-o", a)
    r = 'X'
    r.taint
    a.gsub!('.',r)
    assert_equal(true,a.tainted?) 

    a="hello"
    assert_equal(nil,a.sub!('X','Y'))
  end

  def test_hash
    assert_equal("hello".hash,"hello".hash)
    assert("Hello".hash != "hello".hash)
  end

  def test_hex
    assert_equal(255,"0xff".hex)
    assert_equal(-255,"-0xff".hex)
    assert_equal(255,"ff".hex)
    assert_equal(-255,"-ff".hex)
    assert_equal(0,"-ralph".hex)
    assert_equal(-15,"-fred".hex)
    assert_equal(15,"fred".hex)
  end

  def test_include?
    assert_equal(true, "foobar".include?(?f))
    assert_equal(true, "foobar".include?("foo"))
    assert_equal(false, "foobar".include?("baz"))
    assert_equal(false, "foobar".include?(?z))
  end

  def test_index
    assert_equal(0,"hello".index(?h))
    assert_equal(1,"hello".index("ell"))
    assert_equal(2,"hello".index(/ll./))

    assert_equal(3,"hello".index(?l,3))
    assert_equal(3,"hello".index("l",3))
    assert_equal(3,"hello".index(/l./,3))

    assert_equal(nil,"hello".index(?z,3))
    assert_equal(nil,"hello".index("z",3))
    assert_equal(nil,"hello".index(/z./,3))

    assert_equal(nil,"hello".index(?z))
    assert_equal(nil,"hello".index("z"))
    assert_equal(nil,"hello".index(/z./))
  end

  def test_intern
    assert_equal(:koala, "koala".intern)
    assert(:koala != "Koala".intern)
  end

  def test_length
    assert_equal(0,"".length)
    assert_equal(4,"1234".length)
    assert_equal(6,"1234\r\n".length)
    assert_equal(7,"\0011234\r\n".length)
  end

  def test_ljust
    assert_equal("hello", "hello".ljust(4))
    assert_equal("hello      ", "hello".ljust(11))
  end

  def test_next
    assert_equal("abd","abc".next)
    assert_equal("z","y".next)
    assert_equal("aaa","zz".next)

    assert_equal("124","123".next)
    assert_equal("1000","999".next)

    assert_equal("2000aaa", "1999zzz".next)
    assert_equal("AAAAA000", "ZZZZ999".next)
    assert_equal("*+", "**".next)
  end

  def test_next!
    a="abc"
    assert_equal("abd",a.next!)
    assert_equal("abd",a)

    a="y"
    assert_equal("z",a.next!)
    assert_equal("z",a)

    a="zz"
    assert_equal("aaa",a.next!)
    assert_equal("aaa",a)

    a="123"
    assert_equal("124",a.next!)
    assert_equal("124",a)

    a="999"
    assert_equal("1000",a.next!)
    assert_equal("1000",a)

    a="1999zzz"
    assert_equal("2000aaa", a.next!)
    assert_equal("2000aaa", a)

    a="ZZZZ999"
    assert_equal("AAAAA000", a.next!)
    assert_equal("AAAAA000", a)

    a="**"
    assert_equal("*+", a.next!)
    assert_equal("*+", a)
  end

  def test_oct
    assert_equal(255,"0377".oct)
    assert_equal(255,"377".oct)
    assert_equal(-255,"-0377".oct)
    assert_equal(-255,"-377".oct)
    assert_equal(0,"OO".oct)
    assert_equal(24,"030OO".oct)
  end

  def test_replace
    a="foo"
    assert_equal("f",a.replace("f"))
    a="foo"
    assert_equal("foobar",a.replace("foobar"))
    a="foo"
    a.taint
    b = a.replace("xyz")
    assert_equal("xyz",b)
    assert_equal(true,b.tainted?)
  end

  def test_reverse
    assert_equal("beta","ateb".reverse)
    assert_equal("madamImadam","madamImadam".reverse)
  end

  def test_reverse!
    a="beta"
    assert_equal("ateb",a.reverse!)
    assert_equal("ateb",a)
  end

  def test_rindex
    assert_equal(3,"hello".rindex(?l))
    assert_equal(6,"ell, hello".rindex("ell"))
    assert_equal(7,"ell, hello".rindex(/ll./))

    assert_equal(3,"hello,lo".rindex(?l,3))
    assert_equal(3,"hello,lo".rindex("l",3))
    assert_equal(3,"hello,lo".rindex(/l./,3))

    assert_equal(nil,"hello".rindex(?z,3))
    assert_equal(nil,"hello".rindex("z",3))
    assert_equal(nil,"hello".rindex(/z./,3))

    assert_equal(nil,"hello".rindex(?z))
    assert_equal(nil,"hello".rindex("z"))
    assert_equal(nil,"hello".rindex(/z./))
  end

  def test_rjust
    assert_equal("hello", "hello".rjust(4))
    assert_equal("      hello", "hello".rjust(11))
  end

  def test_scan
    a = "cruel world"
    assert_equal(["cruel","world"],a.scan(/\w+/))
    assert_equal(["cru", "el ","wor"],a.scan(/.../))
    assert_equal([["cru"], ["el "],["wor"]],a.scan(/(...)/))

    res=[]
    a.scan(/\w+/) { |w| res << w }
    assert_equal(["cruel","world"],res)

    res=[]
    a.scan(/.../) { |w| res << w }
    assert_equal(["cru", "el ","wor"],res)

    res=[]
    a.scan(/(...)/) { |w| res << w }
    assert_equal([["cru"], ["el "],["wor"]],res)
  end

  def test_size
    assert_equal(0,"".size)
    assert_equal(4,"1234".size)
    assert_equal(6,"1234\r\n".size)
    assert_equal(7,"\0011234\r\n".size)
  end

  def test_slice
    assert_equal(65,"AooBar".slice(0))
    assert_equal(66,"FooBaB".slice(-1))
    assert_equal(nil,"FooBar".slice(6))
    assert_equal(nil,"FooBar".slice(-7))

    assert_equal("Foo","FooBar".slice(0,3))
    assert_equal("Bar","FooBar".slice(-3,3))
    assert_equal(nil,"FooBar".slice(7,2))     # Maybe should be six?
    assert_equal(nil,"FooBar".slice(-7,10))

    assert_equal("Foo","FooBar".slice(0..2))
    assert_equal("Bar","FooBar".slice(-3..-1))
    assert_equal(nil,"FooBar".slice(6..2))
    assert_equal(nil,"FooBar".slice(-10..-7))

    assert_equal("Foo","FooBar".slice(/^F../))
    assert_equal("Bar","FooBar".slice(/..r$/))
    assert_equal(nil,"FooBar".slice(/xyzzy/))
    assert_equal(nil,"FooBar".slice(/plugh/))

    assert_equal("Foo","FooBar".slice("Foo"))
    assert_equal("Bar","FooBar".slice("Bar"))
    assert_equal(nil,"FooBar".slice("xyzzy"))
    assert_equal(nil,"FooBar".slice("plugh"))
  end

  def test_slice!
    a="AooBar"
    assert_equal(65,a.slice!(0))
    assert_equal("ooBar",a)

    a="FooBar"
    assert_equal(?r,a.slice!(-1))
    assert_equal("FooBa",a)

    a="FooBar"
    assert_exception(IndexError) { a.slice!(6) }
    assert_equal("FooBar",a)
    assert_exception(IndexError) { a.slice!(-7) }
    assert_equal("FooBar",a)

    a="FooBar"
    assert_equal("Foo",a.slice!(0,3))
    assert_equal("Bar",a)
    a="FooBar"
    assert_equal("Bar",a.slice!(-3,3))
    assert_equal("Foo",a)

    a="FooBar"
    assert_exception(IndexError) {a.slice!(7,2)}     # Maybe should be six?
    assert_equal("FooBar",a)
    assert_exception(IndexError) {a.slice!(-7,10)}
    assert_equal("FooBar",a)

    a="FooBar"
    assert_equal("Foo",a.slice!(0..2))
    assert_equal("Bar",a)

    a="FooBar"
    assert_equal("Bar",a.slice!(-3..-1))
    assert_equal("Foo",a)

    a="FooBar"
    assert_exception(RangeError) {a.slice!(6..2)}
    assert_equal("FooBar",a)
    assert_exception(RangeError) {a.slice!(-10..-7)}
    assert_equal("FooBar",a)

    a="FooBar"
    assert_equal("Foo",a.slice!(/^F../))
    assert_equal("Bar",a)

    a="FooBar"
    assert_equal("Bar",a.slice!(/..r$/))
    assert_equal("Foo",a)

    a="FooBar"
    assert_equal(nil,a.slice!(/xyzzy/))
    assert_equal("FooBar",a)
    assert_equal(nil,a.slice!(/plugh/))
    assert_equal("FooBar",a)

    a="FooBar"
    assert_equal("Foo",a.slice!("Foo"))
    assert_equal("Bar",a)

    a="FooBar"
    assert_equal("Bar",a.slice!("Bar"))
    assert_equal("Foo",a)

    a="FooBar"
    assert_equal(nil,a.slice!("xyzzy"))
    assert_equal("FooBar",a)
    assert_equal(nil,a.slice!("plugh"))
    assert_equal("FooBar",a)
  end

  def test_split
    assert_equal(nil,$;)
    assert_equal(["a","b","c"], " a   b\t c ".split)
    assert_equal(["a","b","c"], " a   b\t c ".split(" "))
    assert_equal([" a "," b "," c "], " a | b | c ".split("|"))

    assert_equal(["a","b","c"], "aXXbXXcXX".split(/X./))
    assert_equal(["a","b","c"], "abc".split(//))

    assert_equal(["a|b|c"], "a|b|c".split('|',1))

    assert_equal(["a","b|c"], "a|b|c".split('|',2))
    assert_equal(["a","b","c"], "a|b|c".split('|',3))
    assert_equal(["a","b","c",""], "a|b|c|".split('|',-1))
    assert_equal(["a","b","c","",""], "a|b|c||".split('|',-1))

    assert_equal(["a","", "b", "c"], "a||b|c|".split('|'))
    assert_equal(["a","", "b", "c",""], "a||b|c|".split('|',-1))
  end

  def test_squeeze
    assert_equal("abc","aaabbbbccc".squeeze)
    assert_equal("aa bb cc","aa   bb      cc".squeeze(" "))
    assert_equal("BxTyWz","BxxxTyyyWzzzzz".squeeze("a-z"))
  end

  def test_squeeze!
    a="aaabbbbccc"
    assert_equal("abc",a.squeeze!)
    assert_equal("abc",a)
    a="aa   bb      cc"
    assert_equal("aa bb cc",a.squeeze!(" "))
    assert_equal("aa bb cc",a)

    a="BxxxTyyyWzzzzz"
    assert_equal("BxTyWz",a.squeeze!("a-z"))
    assert_equal("BxTyWz",a)

    a="The quick brown fox"
    assert_equal(nil,a.squeeze!)

  end

  def test_strip
    assert_equal("x","      x        ".strip)
    assert_equal("x"," \n\r\t     x  \t\r\n\n      ".strip)
  end

  def test_strip!
    a="      x        "
    assert_equal("x",a.strip!)
    a=" \n\r\t     x  \t\r\n\n      "
    assert_equal("x",a.strip!)
    a="x"
    assert_equal(nil,a.strip!)
  end

  def test_sub
    assert_equal("h*llo","hello".sub(/[aeiou]/,'*'))
    assert_equal("h<e>llo","hello".sub(/([aeiou])/,'<\1>'))
    assert_equal("104 ello", "hello".sub('.') {
                   |s| s[0].to_s+' '})
    assert_equal("HELL-o", "hello".sub(/(hell)(.)/) {
                   |s| $1.upcase + '-' + $2
                   })
    a = "hello"
    a.taint
    assert_equal(true, (a.sub('.','X').tainted?))
  end

  def test_sub!
    a = "hello"
    a.sub!(/[aeiou]/,'*')
    assert_equal("h*llo",a)

    a = "hello"
    a.sub!(/([aeiou])/,'<\1>')
    assert_equal("h<e>llo",a)

    a = "hello"
    a.sub!('.') { |s| s[0].to_s+' '}
    assert_equal("104 ello", a)

    a = "hello"
    a.sub!(/(hell)(.)/) { |s| $1.upcase + '-' + $2 }
    assert_equal("HELL-o", a)

    a="hello"
    assert_equal(nil,a.sub!('X','Y'))

    r = 'X'
    r.taint
    a.sub!('.',r)
    assert_equal(true,a.tainted?) 
  end

  def test_succ
    assert_equal("abd","abc".succ)
    assert_equal("z","y".succ)
    assert_equal("aaa","zz".succ)

    assert_equal("124","123".succ)
    assert_equal("1000","999".succ)

    assert_equal("2000aaa", "1999zzz".succ)
    assert_equal("AAAAA000", "ZZZZ999".succ)
    assert_equal("*+", "**".succ)
  end

  def test_succ!
    a="abc"
    assert_equal("abd",a.succ!)
    assert_equal("abd",a)

    a="y"
    assert_equal("z",a.succ!)
    assert_equal("z",a)

    a="zz"
    assert_equal("aaa",a.succ!)
    assert_equal("aaa",a)

    a="123"
    assert_equal("124",a.succ!)
    assert_equal("124",a)

    a="999"
    assert_equal("1000",a.succ!)
    assert_equal("1000",a)

    a="1999zzz"
    assert_equal("2000aaa", a.succ!)
    assert_equal("2000aaa", a)

    a="ZZZZ999"
    assert_equal("AAAAA000", a.succ!)
    assert_equal("AAAAA000", a)

    a="**"
    assert_equal("*+", a.succ!)
    assert_equal("*+", a)
  end

  def test_sum
    n="\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001"
    assert_equal(15, n.sum)
    n += "\001"
    assert_equal(16, n.sum(17))
    n[0] = 2
    assert(15 != n.sum)
  end

  def test_swapcase
    assert_equal("hi&LOW","HI&low".swapcase)
  end

  def test_swapcase!
    a="hi&LOW"
    assert_equal("HI&low",a.swapcase!)
    assert_equal("HI&low",a)
    a="$^#^%$#!!"
    assert_equal(nil,a.swapcase!)
    assert_equal("$^#^%$#!!",a)
  end

  def test_to_f
    assert_equal(344.3,"344.3".to_f)
    assert_equal(5.9742e24,"5.9742e24".to_f)
    assert_equal(98.6, "98.6 degrees".to_f)
    assert_equal(0.0, "degrees 100.0".to_f)
  end

  def test_to_i
    assert_equal(1480,"1480ft/sec".to_i)
    assert_equal(0,"speed of sound in water @20C = 1480ft/sec".to_i)
  end

  def test_to_s
    a="me"
    assert_equal(a,a.to_s)
    assert_equal(a.id,a.to_s.id)
  end

  def test_to_str
    a="me"
    assert_equal(a,a.to_s)
    assert_equal(a.id,a.to_s.id)
  end

  def test_tr
    assert_equal("hippo","hello".tr("el","ip"))
    assert_equal("*e**o","hello".tr("^aeiou","*"))
    assert_equal("hal","ibm".tr("b-z","a-z"))
  end

  def test_tr!
    a="hello"
    assert_equal("hippo",a.tr!("el","ip"))
    assert_equal("hippo",a)

    a="hello"
    assert_equal("*e**o",a.tr!("^aeiou","*"))
    assert_equal("*e**o",a)

    a="IBM"
    assert_equal("HAL",a.tr!("B-Z","A-Z"))
    assert_equal("HAL",a)

    a="ibm"
    assert_equal(nil,a.tr!("B-Z","A-Z"))
    assert_equal("ibm",a)
  end

  def test_tr_s
    assert_equal("hypo","hello".tr_s("el","yp"))
    assert_equal("h*o","hello".tr_s("el","*"))
  end

  def test_tr_s!
    a="hello"
    assert_equal("hypo", a.tr_s!("el","yp"))
    assert_equal("hypo", a)

    a="hello"
    assert_equal("h*o",a.tr_s!("el","*"))
    assert_equal("h*o",a)
  end

  def test_unpack
    assert_fail("untested")
  end

  def test_upcase
    assert_equal("HELLO","hElLo".upcase)
    assert_equal("HELLO","hello".upcase)
    assert_equal("HELLO","HELLO".upcase)
    assert_equal("ABC HELLO 123","abc HELLO 123".upcase)
  end

  def test_upcase!
    a="heLlO"
    a.upcase!
    assert_equal("HELLO",a)

    a="HELLO"
    assert_equal(nil,a.upcase!)
    assert_equal("HELLO",a)
  end

  def test_upto
    a="aa"
    start="aa"
    count = 0
    assert_equal("aa", a.upto("zz") {|s|
                   assert_equal(start,s)
                   start.succ!
                   count += 1
                   })
    assert_equal(676,count)
  end

  def test_s_new
    assert_equal("RUBY",String.new("RUBY"))
  end

end

Rubicon::handleTests(TestString) if $0 == __FILE__
