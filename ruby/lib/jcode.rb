# jcode.rb - ruby code to handle japanese (EUC/SJIS) string

$vsave, $VERBOSE = $VERBOSE, FALSE
class String
  printf STDERR, "feel free for some warnings:\n" if $VERBOSE

  PATTERN_SJIS = '[\x81-\x9f\xe0-\xef][\x40-\x7e\x80-\xfc]'
  PATTERN_EUC = '[\xa1-\xfe][\xa1-\xfe]'
  PATTERN_UTF8 = '[\xc0-\xdf][\x80-\xbf]|[\xe0-\xef][\x80-\xbf][\x80-\xbf]'

  RE_SJIS = Regexp.new(PATTERN_SJIS, 'n')
  RE_EUC = Regexp.new(PATTERN_EUC, 'n')
  RE_UTF8 = Regexp.new(PATTERN_UTF8, 'n')

  SUCC = {}
  SUCC['s'] = Hash.new(1)
  for i in 0 .. 0x3f
    SUCC['s'][i.chr] = 0x40 - i
  end
  SUCC['s']["\x7e"] = 0x80 - 0x7e
  SUCC['s']["\xfd"] = 0x100 - 0xfd
  SUCC['s']["\xfe"] = 0x100 - 0xfe
  SUCC['s']["\xff"] = 0x100 - 0xff
  SUCC['e'] = Hash.new(1)
  for i in 0 .. 0xa0
    SUCC['e'][i.chr] = 0xa1 - i
  end
  SUCC['e']["\xfe"] = 2
  SUCC['u'] = Hash.new(1)
  for i in 0 .. 0x7f
    SUCC['u'][i.chr] = 0x80 - i
  end
  SUCC['u']["\xbf"] = 0x100 - 0xbf

  def mbchar?
    case $KCODE[0]
    when ?s, ?S
      self =~ RE_SJIS
    when ?e, ?E
      self =~ RE_EUC
    when ?u, ?U
      self =~ RE_UTF8
    else
      nil
    end
  end

  def end_regexp
    case $KCODE[0]
    when ?s, ?S
      /#{PATTERN_SJIS}$/o
    when ?e, ?E
      /#{PATTERN_EUC}$/o
    when ?u, ?U
      /#{PATTERN_UTF8}$/o
    else
      /.$/o
    end
  end

  alias original_succ! succ!
  private :original_succ!

  alias original_succ succ
  private :original_succ

  def succ!
    reg = end_regexp
    if self =~ reg
      succ_table = SUCC[$KCODE[0,1].downcase]
      begin
	self[-1] += succ_table[self[-1]]
	self[-2] += 1 if self[-1] == 0
      end while self !~ reg
      self
    else
      original_succ!
    end
  end

  def succ
    (str = self.dup).succ! or str
  end

  private

  def _expand_ch str
    a = []
    str.scan(/(.|\n)-(.|\n)|(.|\n)/) do |r|
      if $3
	a.push $3
      elsif $1.length != $2.length
 	next
      elsif $1.length == 1
 	$1[0].upto($2[0]) { |c| a.push c.chr }
      else
 	$1.upto($2) { |c| a.push c }
      end
    end
    a
  end

  def expand_ch_hash from, to
    h = {}
    afrom = _expand_ch(from)
    ato = _expand_ch(to)
    afrom.each_with_index do |x,i| h[x] = ato[i] || ato[-1] end
    h
  end

  HashCache = {}
  TrPatternCache = {}
  DeletePatternCache = {}
  SqueezePatternCache = {}

  public

  def tr!(from, to)
    return self.delete!(from) if to.length == 0

    pattern = TrPatternCache[from] ||= /[#{Regexp::quote(from)}]/
    if from[0] == ?^
      last = /.$/.match(to)[0]
      self.gsub!(pattern, last)
    else
      h = HashCache[from + "::" + to] ||= expand_ch_hash(from, to)
      self.gsub!(pattern) do |c| h[c] end
    end
  end

  def tr(from, to)
    (str = self.dup).tr!(from, to) or str
  end

  def delete!(del)
    self.gsub!(DeletePatternCache[del] ||= /[#{Regexp::quote(del)}]+/, '')
  end

  def delete(del)
    (str = self.dup).delete!(del) or str
  end

  def squeeze!(del=nil)
    pattern =
      if del
	SqueezePatternCache[del] ||= /([#{Regexp::quote(del)}])\1+/
      else
	/(.|\n)\1+/
      end
    self.gsub!(pattern, '\1')
  end

  def squeeze(del=nil)
    (str = self.dup).squeeze!(del) or str
  end

  def tr_s!(from, to)
    return self.delete!(from) if to.length == 0

    pattern = SqueezePatternCache[from] ||= /([#{Regexp::quote(from)}])\1+"/ #"
    if from[0] == ?^
      last = /.$/.match(to)[0]
      self.gsub!(pattern, last)
    else
      h = HashCache[from + "::" + to] ||= expand_ch_hash(from, to)
      self.gsub!(pattern) do h[$1] end
    end
  end

  def tr_s(from, to)
    (str = self.dup).tr_s!(from,to) or str
  end

  def chop!
    self.gsub!(/(?:.|\r?\n)\z/, '')
  end

  def chop
    (str = self.dup).chop! or str
  end

  def jlength
    self.gsub(/[^\Wa-zA-Z_\d]/, ' ').length
  end
  alias jsize jlength

  def jcount(str)
    self.delete("^#{str}").jlength
  end

  def each_char
    if iterator?
      scan(/./m) do |x|
        yield x
      end
    else
      scan(/./m)
    end
  end

end
$VERBOSE = $vsave
