# date2.rb: Written by Tadayoshi Funaba 1998, 1999
# $Id$

class Date

  include Comparable

  IDENT = 2

  MONTHNAMES = [ nil, 'January', 'February', 'March',
    'April', 'May', 'June', 'July', 'August',
    'September', 'October', 'November', 'December' ]

  DAYNAMES = [ 'Sunday', 'Monday', 'Tuesday',
    'Wednesday', 'Thursday', 'Friday', 'Saturday' ]

  ITALY     = 2299161 # 1582-10-15
  ENGLAND   = 2361222 # 1752-09-14
  JULIAN    = false
  GREGORIAN = true

  class << self

    def os? (jd, sg)
      case sg
      when Numeric; jd < sg
      else;         not sg
      end
    end

    def ns? (jd, sg) not os?(jd, sg) end

    def civil_to_jd(y, m, d, sg=GREGORIAN)
      if m <= 2
	y -= 1
	m += 12
      end
      a = (y / 100.0).floor
      b = 2 - a + (a / 4.0).floor
      jd = (365.25 * (y + 4716)).floor +
	(30.6001 * (m + 1)).floor +
	d + b - 1524
      if os?(jd, sg)
	jd -= b
      end
      jd
    end

    def jd_to_civil(jd, sg=GREGORIAN)
      if os?(jd, sg)
	a = jd
      else
	x = ((jd - 1867216.25) / 36524.25).floor
	a = jd + 1 + x - (x / 4.0).floor
      end
      b = a + 1524
      c = ((b - 122.1) / 365.25).floor
      d = (365.25 * c).floor
      e = ((b - d) / 30.6001).floor
      dom = b - d - (30.6001 * e).floor
      if e <= 13
	m = e - 1
	y = c - 4716
      else
	m = e - 13
	y = c - 4715
      end
      return y, m, dom
    end

    def ordinal_to_jd(y, d, sg=GREGORIAN)
      civil_to_jd(y, 1, d, sg)
    end

    def jd_to_ordinal(jd, sg=GREGORIAN)
      y = jd_to_civil(jd, sg)[0]
      doy = jd - civil_to_jd(y - 1, 12, 31, ns?(jd, sg))
      return y, doy
    end

    def jd_to_commercial(jd, sg=GREGORIAN)
      ns = ns?(jd, sg)
      a = jd_to_civil(jd - 3, ns)[0]
      y = if jd >= commercial_to_jd(a + 1, 1, 1, ns) then a + 1 else a end
      w = 1 + (jd - commercial_to_jd(y, 1, 1, ns)) / 7
      d = (jd + 1) % 7
      if d.zero? then d = 7 end
      return y, w, d
    end

    def commercial_to_jd(y, w, d, ns=GREGORIAN)
      jd = civil_to_jd(y, 1, 4, ns)
      (jd - (((jd - 1) + 1) % 7)) +
	7 * (w - 1) +
	(d - 1)
    end

    def clfloor(x, y=1)
      q = (x / y).to_i
      z = (q * y)
      q -= 1 if (y > 0 and x < z) or (y < 0 and x > z)
      r = x - q * y
      return q, r
    end

    def rjd_to_jd(rjd) clfloor(rjd + 0.5) end
    def jd_to_rjd(jd, fr) jd + fr - 0.5 end

    def mjd_to_jd(mjd) mjd + 2400000.5 end
    def jd_to_mjd(jd) jd - 2400000.5 end
    def tjd_to_jd(tjd) tjd + 2440000.5 end
    def jd_to_tjd(jd) jd - 2440000.5 end

    def julian_leap? (y) y % 4 == 0 end
    def gregorian_leap? (y) y % 4 == 0 and y % 100 != 0 or y % 400 == 0 end

    alias_method :leap?, :gregorian_leap?

    def new1(jd=0, sg=ITALY) new(jd, sg) end

    def exist3? (y, m, d, sg=ITALY)
      if m < 0
	m += 13
      end
      if d < 0
	ny, nm = Date.clfloor(y * 12 + m, 12)
	nm,    = Date.clfloor(m + 1, 1)
	la = nil
	31.downto 1 do |z|
	  break if la = exist3?(y, m, z, sg)
	end
	ns = ns?(la, sg)
	d = jd_to_civil(civil_to_jd(ny, nm, 1, ns) + d, ns)[-1]
      end
      jd = civil_to_jd(y, m, d, sg)
      return unless [y, m, d] == jd_to_civil(jd, sg)
      jd
    end

    alias_method :exist?, :exist3?

    def new3(y=-4712, m=1, d=1, sg=ITALY)
      unless jd = exist3?(y, m, d, sg)
	fail ArgumentError, 'invalid date'
      end
      new(jd, sg)
    end

    def exist2? (y, d, sg=ITALY)
      if d < 0
	ny = y + 1
	la = nil
	366.downto 1 do |z|
	  break if la = exist2?(y, z, sg)
	end
	ns = ns?(la, sg)
	d = jd_to_ordinal(ordinal_to_jd(ny, 1, ns) + d, ns)[-1]
      end
      jd = ordinal_to_jd(y, d, sg)
      return unless [y, d] == jd_to_ordinal(jd, sg)
      jd
    end

    def new2(y=-4712, d=1, sg=ITALY)
      unless jd = exist2?(y, d, sg)
	fail ArgumentError, 'invalid date'
      end
      new(jd, sg)
    end

    def existw? (y, w, d, sg=ITALY)
      if d < 0
	d += 8
      end
      if w < 0
	w = jd_to_commercial(commercial_to_jd(y + 1, 1, 1) + w * 7)[1]
      end
      jd = commercial_to_jd(y, w, d)
      return unless ns?(jd, sg)
      return unless [y, w, d] == jd_to_commercial(jd)
      jd
    end

    def neww(y=1582, w=41, d=5, sg=ITALY)
      unless jd = existw?(y, w, d, sg)
	fail ArgumentError, 'invalid date'
      end
      new(jd, sg)
    end

    def today(sg=ITALY)
      new(civil_to_jd(*(Time.now.to_a[3..5].reverse << sg)), sg)
    end

    def once(*ids)
      for id in ids
	module_eval <<-"end;"
	  alias_method :__#{id.to_i}__, #{id}
	  def #{id.id2name}(*args, &block)
	    def self.#{id.id2name}(*args, &block); @__#{id.to_i}__ end
	    @__#{id.to_i}__ = __#{id.to_i}__(*args, &block)
	  end
	end;
      end
    end

    private :once

  end

  def initialize(rjd=0, sg=ITALY) @rjd, @sg = rjd, sg end

  def rjd() @rjd end
  def rmjd() Date.jd_to_mjd(@rjd) end
  def rtjd() Date.jd_to_tjd(@rjd) end

  once :rmjd, :rtjd

  def jd() Date.rjd_to_jd(@rjd)[0] end
  def fr1() Date.rjd_to_jd(@rjd)[1] end
  def mjd() Date.jd_to_mjd(jd) end
  def tjd() Date.jd_to_tjd(jd) end

  once :jd, :fr1, :mjd, :tjd

  def civil() Date.jd_to_civil(jd, @sg) end
  def ordinal() Date.jd_to_ordinal(jd, @sg) end
  def commercial() Date.jd_to_commercial(jd, @sg) end

  once :civil, :ordinal, :commercial

  def year() civil[0] end
  def yday() ordinal[1] end
  def mon() civil[1] end

  alias_method :month, :mon
  once :year, :yday, :mon, :month

  def mday() civil[2] end

  alias_method :day, :mday
  once :mday, :day

  def cwyear() commercial[0] end
  def cweek() commercial[1] end
  def cwday() commercial[2] end

  once :cwyear, :cweek, :cwday

  def wday() (jd + 1) % 7 end

  once :wday

  def os? () Date.os?(jd, @sg) end
  def ns? () Date.ns?(jd, @sg) end

  once :os?, :ns?

  def leap?
    Date.jd_to_civil(Date.civil_to_jd(year, 3, 1, ns?) - 1,
		     ns?)[-1] == 29
  end

  once :leap?

  def sg() @sg end
  def newsg(sg=Date::ITALY) Date.new(@rjd, sg) end

  def italy() newsg(Date::ITALY) end
  def england() newsg(Date::ENGLAND) end
  def julian() newsg(Date::JULIAN) end
  def gregorian() newsg(Date::GREGORIAN) end

  def + (n)
    case n
    when Numeric; return Date.new(@rjd + n, @sg)
    end
    fail TypeError, 'expected numeric'
  end

  def - (x)
    case x
    when Numeric; return Date.new(@rjd - x, @sg)
    when Date;    return @rjd - x.rjd
    end
    fail TypeError, 'expected numeric or date'
  end

  def <=> (other)
    case other
    when Numeric; return @rjd <=> other
    when Date;    return @rjd <=> other.rjd
    end
    fail TypeError, 'expected numeric or date'
  end

  def === (other)
    case other
    when Numeric; return jd == other
    when Date;    return jd == other.jd
    end
    fail TypeError, 'expected numeric or date'
  end

  def >> (n)
    y, m = Date.clfloor(year * 12 + (mon - 1) + n, 12)
    m,   = Date.clfloor(m + 1, 1)
    d = mday
    d -= 1 until jd2 = Date.exist3?(y, m, d, ns?)
    self + (jd2 - jd)
  end

  def << (n) self >> -n end

  def step(limit, step)
    rjd = @rjd
    if (step > 0)
      while rjd <= limit.rjd
	yield Date.new(rjd, @sg)
	rjd += step
      end
    else
      while rjd >= limit.rjd
	yield Date.new(rjd, @sg)
	rjd += step
      end
    end
    self
  end

  def upto(max, &block) step(max, +1, &block) end
  def downto(min, &block) step(min, -1, &block) end

  def succ() self + 1 end

  alias_method :next, :succ

  def eql? (other) Date === other and self == other end
  def hash() Date.clfloor(@rjd)[0] end
  def inspect() format('#<Date: %s,%s>', @rjd, @sg) end
  def to_s() format('%.4d-%02d-%02d', year, mon, mday) end

  def _dump(limit) Marshal.dump([@rjd, @sg], -1) end
  def Date._load(str) Date.new(*Marshal.load(str)) end

end
