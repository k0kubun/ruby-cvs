# $Id$

=begin
= Pretty-printer for Ruby objects.

== Which seems better?

non-pretty-printed output by (({p})) is:
  #<PP:0x806486c @stack=[], @nest=[0], @buf=#<PP::Group:0x8064844 @tail=0, @singleline_length=9, @buf=[#<PP::Group:0x80647cc @tail=0, @singleline_length=9, @buf=[#<PP::Text:0x8064768 @text="[">, #<PP::Group:0x80646c8 @tail=1, @singleline_length=1, @buf=[#<PP::Text:0x8064664 @text="1">]>, #<PP::Text:0x80646b4 @text=",">, #<PP::Breakable:0x8064650 @indent=1, @sep=" ">, #<PP::Group:0x8064614 @tail=1, @singleline_length=1, @buf=[#<PP::Text:0x80645b0 @text="2">]>, #<PP::Text:0x8064600 @text=",">, #<PP::Breakable:0x806459c @indent=1, @sep=" ">, #<PP::Group:0x8064560 @tail=1, @singleline_length=1, @buf=[#<PP::Text:0x80644fc @text="3">]>, #<PP::Text:0x806472c @text="]">]>]>>

pretty-printed output by (({pp})) is:
  #<PP:0x403279c
   @buf=
    #<PP::Group:0x4032666
     @buf=
      [#<PP::Group:0x4032666
        @buf=
         [#<PP::Text:0x40326de @text="[">,
          #<PP::Group:0x4032666
           @buf=[#<PP::Text:0x40326de @text="1">],
           @singleline_length=1,
           @tail=1>,
          #<PP::Text:0x40326de @text=",">,
          #<PP::Breakable:0x40326b6 @indent=1, @sep=" ">,
          #<PP::Group:0x4032666
           @buf=[#<PP::Text:0x40326de @text="2">],
           @singleline_length=1,
           @tail=1>,
          #<PP::Text:0x40326de @text=",">,
          #<PP::Breakable:0x40326b6 @indent=1, @sep=" ">,
          #<PP::Group:0x4032666
           @buf=[#<PP::Text:0x40326de @text="3">],
           @singleline_length=1,
           @tail=1>,
          #<PP::Text:0x40326de @text="]">],
        @singleline_length=9,
        @tail=0>],
     @singleline_length=9,
     @tail=0>,
   @nest=[0],
   @stack=[]>

I like the latter.  If you do too, this library is for you.

== Usage

: pp(obj)
    output ((|obj|)) to (({$>})) in pretty printed format.

== Customized output
To define your customized pretty printing function for your class,
redefine a method (({pretty_print(((|pp|)))})) in the class.
It takes an argument ((|pp|)) which is an instance of the class ((<PP>)).
The method should use PP#text, PP#breakable, PP#nest, PP#group and
PP#pp to print the object.

= PP
== super class
((<PrettyPrint>))

== class methods
--- PP.pp(obj[, width[, out]])
    outputs ((|obj|)) to ((|out|)) in pretty printed format of
    ((|width|)) columns in width.

--- PP.sharing_detection
    returns the sharing detection flag as boolean value.
    It is false by default.

--- PP.sharing_detection = value
    sets the sharing detection flag.

== methods
--- pp(obj)
    adds ((|obj|)) to the pretty printing buffer
    using Object#pretty_print or Object#pretty_print_cycled.

    Object#pretty_print_cycled is used when ((|obj|)) is already
    printed, a.k.a the object reference chain has a cycle.

= Object
--- pretty_print(pp)
    is a default pretty printing method for general objects.

    If (({self})) has a customized (redefined) (({inspect})) method,
    the result of (({self.inspect})) is used but it obviously has no
    line break hints.

    This module provides predefined pretty_print() methods for some of
    the most commonly used built-in classes for convenience.

--- pretty_print_cycled(pp)
    is a default pretty printing method for general objects that are
    detected as part of a cycle.
=end

require 'prettyprint'

module Kernel
  private
  def pp(*objs)
    objs.each {|obj|
      PP.pp(obj)
    }
  end
end

class PP < PrettyPrint
  def PP.pp(obj, width=79, out=$>)
    pp = PP.new
    pp.guard_inspect_key {pp.pp obj}
    pp.format(out, width)
    out << "\n"
  end

  @@sharing_detection = false
  def PP.sharing_detection
    return @@sharing_detection
  end

  def PP.sharing_detection=(val)
    @@sharing_detection = val
  end

  def initialize
    super
    @sharing_detection = @@sharing_detection
  end

  InspectKey = :__inspect_key__

  def guard_inspect_key
    if Thread.current[InspectKey] == nil
      Thread.current[InspectKey] = []
    end

    save = Thread.current[InspectKey]

    begin
      Thread.current[InspectKey] = []
      yield
    ensure
      Thread.current[InspectKey] = save
    end
  end

  def pp(obj)
    id = obj.__id__

    if Thread.current[InspectKey].include? id
      group {obj.pretty_print_cycled self}
      return
    end

    begin
      Thread.current[InspectKey] << id
      group {obj.pretty_print self}
    ensure
      Thread.current[InspectKey].pop unless @sharing_detection
    end
  end

  def pp_object(obj)
    group {
      text '#<'
      nest(1) {
	text obj.class.name
	text ':'
	text sprintf('0x%x', obj.__id__)
	first = true
	obj.instance_variables.sort.each {|v|
	  if first
	    first = false
	  else
	    text ','
	  end
	  breakable
	  group {
	    text v
	    text '='
	    nest(1) {
	      breakable ''
	      pp(obj.instance_eval v)
	    }
	  }
	}
      }
      text '>'
    }
  end

  def pp_hash(obj)
    group {
      text '{'
      nest(1) {
	first = true
	obj.each {|k, v|
	  if first
	    first = false
	  else
	    text ","
	    breakable
	  end
	  group {
	    pp k
	    text '=>'
	    nest(1) {
	      group {
		breakable ''
		pp v
	      }
	    }
	  }
	}
      }
      text '}'
    }
  end

  module ObjectMixin
    # 1. specific pretty_print
    # 2. specific inspect
    # 3. generic pretty_print

    Key = :__pp_instead_of_inspect__

    def pretty_print(pp)
      # specific pretty_print is not defined, try specific inspect.
      begin
        old = Thread.current[Key]
	result1 = sprintf('#<%s:0x%x pretty_printed>', self.class.name, self.__id__)
        Thread.current[Key] = [pp, result1]
	result2 = ObjectMixin.pp_call_inspect(self)
      ensure
        Thread.current[Key] = old
      end

      unless result1.equal? result2
        pp.text result2
      end
    end

    def ObjectMixin.pp_call_inspect(obj); obj.inspect; end
    CallInspectFrame = "#{__FILE__}:#{__LINE__ - 1}:in `pp_call_inspect'"

    def inspect
      if CallInspectFrame == caller[0]
	# specific inspect is also not defined, use generic pretty_print. 
	pp, result = Thread.current[Key]
	pp.pp_object(self)
	result
      else
	super
      end
    end

    def pretty_print_cycled(pp)
      pp.text sprintf("#<%s:0x%x", self.class.name, self.__id__)
      pp.breakable
      pp.text sprintf("...>")
    end
  end
end

[Numeric, FalseClass, TrueClass, Module].each {|c|
  c.class_eval {
    def pretty_print(pp)
      pp.text self.to_s
    end
  }
}

class Array
  def pretty_print(pp)
    pp.text "["
    pp.nest(1) {
      first = true
      self.each {|v|
	if first
	  first = false
	else
	  pp.text ","
	  pp.breakable
	end
	pp.pp v
      }
    }
    pp.text "]"
  end

  def pretty_print_cycled(pp)
    pp.text(empty? ? '[]' : '[...]')
  end
end

class Hash
  def pretty_print(pp)
    pp.pp_hash self
  end

  def pretty_print_cycled(pp)
    pp.text(empty? ? '{}' : '{...}')
  end
end

class << ENV
  def pretty_print(pp)
    pp.pp_hash self
  end
end

class Struct
  def pretty_print(pp)
    pp.text sprintf("#<%s", self.class.name)
    pp.nest(1) {
      first = true
      self.members.each {|member|
	if first
	  first = false
	else
	  pp.text ","
	end
	pp.breakable
	pp.group {
	  pp.text member.to_s
	  pp.text '='
	  pp.nest(1) {
	    pp.breakable ''
	    pp.pp self[member]
	  }
	}
      }
    }
    pp.text ">"
  end

  def pretty_print_cycled(pp)
    pp.text sprintf("#<%s:...>", self.class.name)
  end
end

class Range
  def pretty_print(pp)
    pp.pp self.begin
    pp.breakable ''
    pp.text(self.exclude_end? ? '...' : '..')
    pp.breakable ''
    pp.pp self.end
  end
end

class File
  class Stat
    def pretty_print(pp)
      require 'etc.so'
      pp.nest(1) {
	pp.text "#<"
	pp.text self.class.name
	pp.breakable; pp.text "dev="; pp.pp self.dev; pp.text ','
	pp.breakable; pp.text "ino="; pp.pp self.ino; pp.text ','
	pp.breakable
	pp.group {
	  m = self.mode
	  pp.text "mode="; pp.pp m
	  pp.breakable
	  pp.text sprintf("(0%o %s %c%c%c%c%c%c%c%c%c)",
	    m, self.ftype,
	    (m & 0400 == 0 ? ?- : ?r),
	    (m & 0200 == 0 ? ?- : ?w),
	    (m & 0100 == 0 ? (m & 04000 == 0 ? ?- : ?S) :
	                     (m & 04000 == 0 ? ?x : ?s)),
	    (m & 0040 == 0 ? ?- : ?r),
	    (m & 0020 == 0 ? ?- : ?w),
	    (m & 0010 == 0 ? (m & 02000 == 0 ? ?- : ?S) :
	                     (m & 02000 == 0 ? ?x : ?s)),
	    (m & 0004 == 0 ? ?- : ?r),
	    (m & 0002 == 0 ? ?- : ?w),
	    (m & 0001 == 0 ? (m & 01000 == 0 ? ?- : ?T) :
	                     (m & 01000 == 0 ? ?x : ?t)))
	  pp.text ','
	}
	pp.breakable; pp.text "nlink="; pp.pp self.nlink; pp.text ','
	pp.breakable
	pp.group {
	  pp.text "uid="; pp.pp self.uid
	  begin
	    name = Etc.getpwuid(self.uid).name
	    pp.breakable; pp.text "(#{name})"
	  rescue ArgumentError
	  end
	  pp.text ','
	}
	pp.breakable
	pp.group {
	  pp.text "gid="; pp.pp self.gid
	  begin
	    name = Etc.getgrgid(self.gid).name
	    pp.breakable; pp.text "(#{name})"
	  rescue ArgumentError
	  end
	  pp.text ','
	}
	pp.breakable; pp.text "rdev="; pp.pp self.rdev; pp.text ','
	pp.breakable; pp.text "size="; pp.pp self.size; pp.text ','
	pp.breakable; pp.text "blksize="; pp.pp self.blksize; pp.text ','
	pp.breakable; pp.text "blocks="; pp.pp self.blocks; pp.text ','
	pp.breakable
	pp.group {
	  t = self.atime
	  pp.text "atime="; pp.pp t
	  pp.breakable; pp.text "(#{t.tv_sec})"
	  pp.text ','
	}
	pp.breakable
	pp.group {
	  t = self.mtime
	  pp.text "mtime="; pp.pp t
	  pp.breakable; pp.text "(#{t.tv_sec})"
	  pp.text ','
	}
	pp.breakable
	pp.group {
	  t = self.ctime
	  pp.text "ctime="; pp.pp t
	  pp.breakable; pp.text "(#{t.tv_sec})"
	}
	pp.text ">"
      }
    end
  end
end

class Object
  include PP::ObjectMixin
end

[Numeric, Symbol, FalseClass, TrueClass, NilClass, Module].each {|c|
  c.class_eval {
    alias :pretty_print_cycled :pretty_print
  }
}

if __FILE__ == $0
  require 'runit/testcase'
  require 'runit/cui/testrunner'

  class PPTest < RUNIT::TestCase
    def test_list0123_12
      assert_equal("[0, 1, 2, 3]\n", PP.pp([0,1,2,3], 12, ''))
    end

    def test_list0123_11
      assert_equal("[0,\n 1,\n 2,\n 3]\n", PP.pp([0,1,2,3], 11, ''))
    end
  end

  class HasInspect
    def initialize(a)
      @a = a
    end

    def inspect
      return "<inspect:#{@a.inspect}>"
    end
  end

  class HasPrettyPrint
    def initialize(a)
      @a = a
    end

    def pretty_print(pp)
      pp.text "<pretty_print:"
      pp.pp @a
      pp.text ">"
    end
  end

  class HasBoth
    def initialize(a)
      @a = a
    end

    def inspect
      return "<inspect:#{@a.inspect}>"
    end

    def pretty_print(pp)
      pp.text "<pretty_print:"
      pp.pp @a
      pp.text ">"
    end
  end

  class PPInspectTest < RUNIT::TestCase
    def test_hasinspect
      a = HasInspect.new(1)
      assert_equal("<inspect:1>\n", PP.pp(a, 79, ''))
    end

    def test_hasprettyprint
      a = HasPrettyPrint.new(1)
      assert_equal("<pretty_print:1>\n", PP.pp(a, 79, ''))
    end

    def test_hasboth
      a = HasBoth.new(1)
      assert_equal("<pretty_print:1>\n", PP.pp(a, 79, ''))
    end
  end

  class PPCycleTest < RUNIT::TestCase
    def test_array
      a = []
      a << a
      assert_equal("[[...]]\n", PP.pp(a, 79, ''))
    end

    def test_hash
      a = {}
      a[0] = a
      assert_equal("{0=>{...}}\n", PP.pp(a, 79, ''))
    end

    S = Struct.new("S", :a, :b)
    def test_struct
      a = S.new(1,2)
      a.b = a
      assert_equal("#<Struct::S a=1, b=#<Struct::S:...>>\n", PP.pp(a, 79, ''))
    end

    def test_object
      a = Object.new
      a.instance_eval {@a = a}
      hex = '0x' + ('%x'%a.id)
      assert_equal("#<Object:#{hex} @a=#<Object:#{hex} ...>>\n", PP.pp(a, 79, ''))
    end

    def test_withinspect
      a = []
      a << HasInspect.new(a)
      assert_equal("[<inspect:[...]>]\n", PP.pp(a, 79, ''))
    end
  end

  RUNIT::CUI::TestRunner.run(PPTest.suite)
  RUNIT::CUI::TestRunner.run(PPInspectTest.suite)
  RUNIT::CUI::TestRunner.run(PPCycleTest.suite)
end