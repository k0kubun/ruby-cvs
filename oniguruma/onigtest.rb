#!/usr/local/bin/ruby -Ke
# EUC-JP version
#
# ���Υ�����ץȤϡ������Ĥ�������ɽ���˥ǡ�����Ŭ�Ѥ�����̤�
# ɸ����Ϥ˽��Ϥ�������Ǥ���
# ���ꥸ�ʥ��Ruby�Ǽ¹Ԥ�����̤ȡ�diff����Ӥ��Ƥ���������
#

regs = [
  //, /^/, /$/, /^$/, /a/, /a*/, /a+/, /a?/, /ab/, /a*b/,
  /ab*/, /^a/, /^b/, /a$/, /ab$/, /^ab$/, /^a*$/, /^a*b*$/, /ab?$/, /^b?a?$/,
  /a./, /../, /a.*/, /.*b/, /.*a+/, /(..)+/, /(.b)?/, /(a..)*b/,
  /()/, /(a)/, /(ab)/, /(())/, /(a(b))/, /a(b)/, /a((b))/, /(a)(b)/,
  /(a*)?/, /(ab+)(ba*)/, /((((((((((ab))))))))))/, /a()b()/,
  /a{0,0}/, /a{0,10}/, /a{2,11}/, /a{0,}/, /a{1,}/, /a{3}/,
  /a{3,5}?/, /(a?){3}{5,8}/, /.*?/, /b*?/, /a+?/, /a??ab/, /.*123/,
  /\A/, /\Ab/, /\Z/, /\z/, /\A\Z/, /\Aa\z/, /a\Z/, /bb\z/,
  /\w/, /\W/, /\w+/, /\W?/, /\d+/, /\D*/, /\s\w/, /\s+\W*/,
  /\ba/, /\Ba/, /\b\B/, /\d*\B\W/, /a\x61/, /\0141b/, /a\x61a/, /\01412b/,
  /[a]/, /[ab]/, /[AaBb]/, /[a-y]/, /[A-F]/, /[ab]*/, /[bc]+/,
  /[-]/, /[b-]/, /[-b]/, /((?i)a)b/, /(?:(?i)a|b)a/, /(a(?i)(a|b*))a/,
  /[^ab]/, /[\w]+/, /[^\w\s]+/, /[^^ab]+/, /[a\\b*!@[]]+/, /[-^+{}]+/,
  /anonymous/, /URL/, /(URL|Ruby)/, /(aa|ccc|bbbb)+/,
  /aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/,
  /[a[:blank:]]*b/, /[[:xdigit:][:alpha:]]+/, /[[:graph:]]+/,
  /ab(#This is a comment)/, /ab(?=c)/, /(?=aa)aaaaa/, /(?!aa).bbb/,
  /123(?!4)/, /a(?>aaa)b/, /a(?>b)/, /(?:bb)+/, /(?:)cccc/, /(?:a(?:b))c/,
  /aaa(?=bbb(?=b))/, /a(?>c(?=d))/, /a(?!c(?>bb))/, /a(?>b(?!c))/,
  /(?i:Aa)/, /(?-i:Ab)/, /(?i:a(?-i:A)a)/,
  /\1/, /(a)\1/, /(a(b)\1)\2+/, /(a(.+(b)))\2/, /\G(bb).*\1+/, /(?=ab)((a)\2)/,
  /(?=(b.))a\1/, /(?!a(a)b)\1/, /(?>(.a))b\1/,
  /��/, /����/, /����*/, /(����)+/, /(a|b|��)a+/, /��������Ϻ/,
  /[����]/, /[b������]+/, /[^a��]��/, /[��-��]*$/, /(��|��)\1/,
  /\265\264\274\326/,
  %r{\G((?!/\*)\S+?(?=/\*|\s|\z)|(?:/\*[\s\S]*?\*/|\s+)+(?!(?:/\*[\s\S]*?\*/|\s+)*\z))}, # akr.

  /END_OF_PATTERN./

# OK, but different spec. with GNU regex
# /[\265\264-\274\326]/,

# OK, but different spec. (or bug?) of GNU regex in ignore case mode($=).
# /(?m:b.a)/, /(?-m:b.a)/,
# /(?x: aaa bbb)/, /(?x:      a .. # comment
#                             b)/,

# OK, but GNU regex bug pattern.
#  /b{1,2}+/, /(ab){1,2}{3}/,
]


LongString1 = <<EOS1
--- From README of Ruby ---

* What is Ruby

Ruby is the interpreted scripting language for quick and
easy object-oriented programming.  It has many features to
process text files and to do system management tasks (as in
Perl).  It is simple, straight-forward, and extensible.


* Features of Ruby

  + Simple Syntax
  + *Normal* Object-Oriented features(ex. class, method calls)
  + *Advanced* Object-Oriented features(ex. Mix-in, Singleton-method)
  + Operator Overloading
  + Exception Handling
  + Iterators and Closures
  + Garbage Collection
  + Dynamic Loading of Object files(on some architecture)
  + Highly Portable(works on many UNIX machines, and on DOS,
    Windows, Mac, BeOS etc.)


* How to get Ruby

The Ruby distribution can be found on:

  ftp://ftp.ruby-lang.org/pub/ruby/

You can get it by anonymous CVS.  How to check out is:

  $ cvs -d :pserver:anonymous@cvs.ruby-lang.org:/src login
  (Logging in to anonymous@cvs.ruby-lang.org)
  CVS password: anonymous
  $ cvs -z4 -d :pserver:anonymous@cvs.ruby-lang.org:/src checkout ruby


* Ruby home-page

The URL of the Ruby home-page is:

   http://www.ruby-lang.org/
EOS1


LongString2 = <<EOS2

<�ᶷ���>

2002/02/25
�Ĥ�������ɽ���饤�֥�� "����"������������

��ǯ�ɤ���ܤ���Ǻ��ΤȤ��������ɤ��ä��Τϡ�
�ֿ�̵������ܤ����(�Ͻ�ʸ��)��
�ֻ�������Ϻ���ͤ������� ���פϡ����Ĥ�Ԥ��ܲ����֤��Ƥʤ��ä��Τǡ�
�ޤ���äƤ��ʤ���


GNU regex�⡢����äȤ狼��ʤ��Ȥ���������ʤ���

  /a{1,2}{3}/.match("aaa")�����Ԥ���Х��ϡ�/a{3,6}/�Ƚ�ľ���Ф�������
  ���Ѿ������ʤ���...

  m = /(?m:a)/i.match("A")���ޥå����ʤ��Τϡ�(?m:)�Υ��ץ��������Ȥ���
  ���ΤΥ��ץ�������Ǥ���i��̵���ˤʤäƤ��뤿��������Ȼפä����ɡ�
  m = /(?m:a)/i.match("a")��ޥå����ʤ�������顢�櫓�狼��ʤ��衣

�����Ǹ��äƤ���GNU regex�äơ�Ruby���Ȥ߹��ޤ�Ƥ������ܸ첽�С������ǡ�
��ʪ�Ϥ�äȥС�����󤬾夬�äƤ�����������ɡ�

�Ǥ⡢���֤Τۤ�����äȲ�������ʡ���
����������ä��Τ���������ʡ���
���ʤ��ƻȤ������ʤ��衣(�ޥ���)

EOS2

# data
strings = [
  "", "a", " ", "aa", "ab", "aabb", "aaa", "aaabbb", "abab!ababba b",
  "A", "AAAAAAA", "BBBBB^", "ABABAB", "B AAAAA", "BBBBBBBBBBBBB",
  "aaaaaaaaaa", "bbbbbbbbbbbbbbbbbbb^b", "a a b c aa bb cc aaa bbb",
  "b\na", "bbbbbbbbbbb\naaaaaaaaaaa\n/* aaa */",
  "aaaa1234567890", "1234567890abcdefghijklmnopqrstuvwxyz",
  "@!\"\#$%&'()~=~|AAAACCCCC-----DDDDDD[]{}\\^-`*:_?<>,.+;",
  "@!\"\#$%&'()~=~|[]{}\\^-`*:_?<>,.+;abc",
  " bc aaa /*bbb*/ /* ccc sss */ sdddd sssss /**/ a /* ***** sasa*/ // xxxx.ss ",
  'http://user:passwd@www.din.or.jp/~ohzaki/perl.htm#URI',
  "��������������������abc��������567890",
  "���ּּּּּּּּּּ�/*  */Oni Guruma",
  "���ֵ��ֵ��ֵ��ֵ��ֵ��ֵ��ֵ��ֵ��ֵ��ֵ��ֵ���",
  "aaaavvv vv���ֵ���x���ֵ���x����x��x�ֵ��ֵ��ֵ���a���ֵ��ֵ���z",
  LongString1,
  LongString2
]


def result(i, j, m, g, r)
  printf("%2d:%2d: ", i, j)
  if (m)
    printf("(%d) ", m.length)
    for k in (0...m.length)
      printf(", ") if (k > 0)
      if (m[k])
	printf("'%s' <%d:%d>", m[k], m.begin(k), m.end(k))
      else
	printf("nil")
      end
    end
    printf(" gsub:%s rindex:%s\n", g, r)
  else
    printf("no match\n")
  end
end

def test(strings, regs)
  for i in (0...strings.length)
    for j in (0...regs.length)
      m = regs[j].match(strings[i])
      g = strings[i].gsub(regs[j], '\1"')
      r = strings[i].rindex(regs[j])
      result(i, j, m, g, r)
    end
  end
end

####
if (ARGV.length > 0)
  n = ARGV[0].to_i
  p(strings[n]) if n < strings.length
  p(regs[n])    if n < regs.length
  exit 0
end

repeat_num = 1
repeat_num_ignore_case = 1

for i in 1..repeat_num
  test(strings, regs)
end

# ignore case test
$= = true
for i in 1..repeat_num_ignore_case
  test(strings, regs)
end

# end of script