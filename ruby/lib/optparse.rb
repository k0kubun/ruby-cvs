# optparse library, not octopus.

=begin
= Summary
Library for command line option analysis.

features:
(1) It is possible <option switch of a short form and a long form> to
    exist together. It is also possible in one to bring the switch of
    a short form together.
(2) It is possible to write bringing specification and the handler of
    the switch together respectively in one place.
(3) The argument of the switch is converted into the class which
    automatically specifies it.
(4) The option summary can be made.
(5) The option can be added on the way later.

=end #'#"#`#
# Not yet (;_;)
=begin

== Class tree
   * ((<OptionParser>)) front end
   * ((<OptionParser::Switch>)) each switches
   * ((<OptionParser::List>)) options list
   * ((<OptionParser::ParseError>)) errors on parsing
     * ((<OptionParser::AmbiguousOption>))
     * ((<OptionParser::NeedlessArgument>))
     * ((<OptionParser::MissingArgument>))
     * ((<OptionParser::InvalidOption>))
     * ((<OptionParser::InvalidArgument>))
       * ((<OptionParser::AmbiguousArgument>))

== Object relations
   +--------------+
   | OptionParser |<>-----+
   +--------------+       |          	         +--------+
                          |          	       ,-| Switch |
        on_head -------->+---------------+    /  +--------+
        accept/reject -->| List        	 |<|>-
                         |             	 |<|>-  +----------+
        on ------------->+---------------+    `-| argument |
                         :             	 :      |  class   |
                         +---------------+      |==========|
        on_tail -------->|             	 |      |pattern   |
                         +---------------+      |----------|
   OptionParser.accept ->| DefaultList 	 |      |converter |
                reject   |(shared between|      +----------+
                         | all instances)|
                         +---------------+

=end #'#"#`#

=begin
= Classes & Modules
=end #'#"#`#

class OptionParser
  RCSID = %w$Id$[1..-1].each {|s| s.freeze}.freeze
  Version = (RCSID[1].split('.').collect {|s| s.to_i}.extend(Comparable).freeze if RCSID[1])
  LastModified = (Time.gm(*RCSID[2, 2].join('-').scan(/\d+/).collect {|s| s.to_i}) if RCSID[2])
  Release = RCSID[2]

  NoArgument = [NO_ARGUMENT = :NONE, nil].freeze
  RequiredArgument = [REQUIRED_ARGUMENT = :REQUIRED, true].freeze
  OptionalArgument = [OPTIONAL_ARGUMENT = :OPTIONAL, false].freeze

=begin private
== ((:OptionParser::Completion:))
Keyword completion module.
=end #'#"#`#
  module Completion
=begin private
--- OptionParser::Completion#complete(key[, pat])
    Searches ((|key|)), or ((|pat|)) with completion if not found.
    :Parameters:
      : ((|key|))
        keyword to search.
      : ((|pat|))
        completion pattern.
=end #'#"#`#
    def complete(key, pat = nil)
      pat ||= Regexp.new('\A' + Regexp.quote(key).gsub(/\w+(?=.)/, '\&\w*'), true)
      canon, sw, k, v = nil
      each do |k, *v|
	(if Regexp === k
	   k === key
	 else
	   pat === (defined?(k.id2name) ? k.id2name : k)
	 end) or next
	v << k if v.empty?
	unless canon
	  canon, sw = k, v
	else
	  throw :ambiguous, key unless sw == v
	end
      end
      if canon
	block_given? or return key, *sw
	yield(key, *sw)
      end
    end

=begin private
--- OptionParser::Completion#convert(opt, *val)
    Extracts the first element from result of
    ((<OptionParser::Completion#complete>)).
=end #'#"#`#
    def convert(opt = nil, val = nil, *)
      val
    end
  end

=begin private
== ((:OptionParser::OptionMap:))
Map from option/keyword string to object with completion.
=== Superclass
(({Hash}))
=== Including modules
((<OptionParser::Completion>))
=end #'#"#`#
  class OptionMap < Hash
    include Completion
  end


=begin
== ((:OptionParser::Switch:))
Individual switch class.
=end #'#"#`#
  class Switch
    attr_reader :pattern, :conv, :short, :long, :arg, :desc, :block

=begin private
=== Class methods
=end
=begin private
--- OptionParser::Switch.guess(arg)
    Guesses argument style from ((|arg|)).
    Returns corresponding ((<OptionParser::Switch>)) subclass.
=end #'#"#`#
    def self.guess(arg)
      case arg
      when ""
        self
      when /\A[=\s]?\[/
        Switch::OptionalArgument
      else
        Switch::RequiredArgument
      end
    end

=begin private
--- OptionParser::Switch.new
=end #'#"#`#
    def initialize(pattern = nil, conv = nil,
		   short = nil, long = nil, arg = nil,
		   desc = ([] if short or long), block = Proc.new)
      @pattern, @conv, @short, @long, @arg, @desc, @block =
	pattern, conv, short, long, arg, desc, block
    end

=begin
=== Instance methods
=end
=begin private
--- OptionParser::Switch#parse_arg(arg) {non-serious error handler}
    Parses argument and returns rest of ((|arg|)), and matched portion
    to the argument pattern.
    :Parameters:
      : ((|arg|))
        option argument to be parsed.
      : (({block}))
        yields when the pattern doesn't match sub-string.
=end #'#"#`#
    def parse_arg(arg)
      pattern or return nil, arg
      unless m = pattern.match(arg)
	yield(InvalidArgument, arg)
	return arg, nil
      end
      if String === m
	m = [s = m]
      else
	m = m.to_a
	s = m[0]
	return nil, *m unless String === s
      end
      raise InvalidArgument, arg unless arg.rindex(s, 0)
      return nil, *m if s.length == arg.length
      yield(InvalidArgument, arg) # didn't match whole arg
      return arg[s.length..-1], *m
    end
    private :parse_arg

=begin
--- OptionParser::Switch#parse(arg, val) {semi-error handler}
    Parses argument, convert and returns ((|arg|)), ((|block|)) and
    result of conversion.
    : Arguments to ((|@conv|))
      substrings matched to ((|@pattern|)), ((|$&|)), ((|$1|)),
      ((|$2|)) and so on.
    :Parameters:
      : ((|arg|))
        argument string follows the switch.
      : ((|val|))
        following argument.
      : (({block}))
        (({yields})) at semi-error condition, instead of raises exception.
=end #'#"#`#
    def parse(arg, *val)
      if block
	val = conv.yield(*val) if conv
	return arg, block, val
      else
	return arg, nil
      end
    end

=begin private
--- OptionParser::Switch#summarize(sdone, ldone, width, max, indent)
    Makes summary strings.
    :Parameters:
      : ((|sdone|))
        already summarized short style options keyed hash.
      : ((|ldone|))
        already summarized long style options keyed hash.
      : ((|width|))
        width of left side, option part. in other word, right side,
        description part strings start at ((|width|)) column.
      : ((|max|))
        maximum width of left side, options are filled within ((|max|)) columns.
      : ((|indent|))
        prefix string indents each summarized lines.
      : (({block}))
        to be passed each lines(without newline).
=end #'#"#`#
    def summarize(sdone = [], ldone = [], width = 1, max = width - 1, indent = "")
      sopts, lopts, s = [], [], nil
      @short.each {|s| sdone.fetch(s) {sopts << s}; sdone[s] = true} if @short
      @long.each {|s| ldone.fetch(s) {lopts << s}; ldone[s] = true} if @long
      return if sopts.empty? and lopts.empty? # completely hidden

      left = [sopts.join(', ')]
      right = desc.dup

      while s = lopts.shift
	l = left[-1].length + s.length
	l += arg.length if left.size == 1 && arg
	l < max or left << ''
	left[-1] << if left[-1].empty? then ' ' * 4 else ', ' end << s
      end

      left[0] << arg if arg
      mlen = left.collect {|s| s.length}.max.to_i
      while mlen > width and l = left.shift
	mlen = left.collect {|s| s.length}.max.to_i if l.length == mlen
	yield(indent + l)
      end

      while (l = left.shift; r = right.shift; l or r)
	l = l.to_s.ljust(width) + ' ' + r if r and !r.empty?
	yield(indent + l)
      end

      self
    end

=begin private
== Switch classes
=end #'#"#`#

=begin private
=== ((:OptionParser::Switch::NoArgument:))
Switch that takes no arguments.
==== Superclass
((<OptionParser::Switch>))
==== Instance methods
--- OptionParser::Switch::NoArgument#parse
    Raises an exception if any arguments given.
=end #'#"#`#
    class NoArgument < self
      def parse(arg, argv, &error)
	yield(NeedlessArgument, arg) if arg
	super(arg)
      end
    end

=begin private
=== ((:OptionParser::Switch::RequiredArgument:))
Switch that takes an argument.
==== Superclass
((<OptionParser::Switch>))
==== Instance methods
--- OptionParser::Switch::RequiredArgument#parse
    Raises an exception if argument is not present.
=end #'#"#`#
    class RequiredArgument < self
      def parse(arg, argv, &error)
	unless arg
	  raise MissingArgument if argv.empty?
	  arg = argv.shift
	end
	super(*parse_arg(arg, &error))
      end
      def self.guess(arg)
	self >= (t = super) or
	  raise ArgumentError, "#{arg}: incompatible argument styles\n  #{self}, #{t}"
	t
      end
    end

=begin private
=== ((:OptionParser::Switch::OptionalArgument:))
Switch that can omit argument.
==== Superclass
((<OptionParser::Switch>))
==== Instance methods
--- OptionParser::Switch::OptionalArgument#parse
    Parses argument if given, or uses default value.
=end #'#"#`#
    class OptionalArgument < self
      def parse(arg, argv, &error)
	if arg
	  super(*parse_arg(arg, &error))
	else
	  super(arg)
	end
      end
      def self.guess(arg)
	self >= (t = super) or
	  raise ArgumentError, "#{arg}: incompatible argument styles\n  #{self}, #{t}"
	t
      end
    end
  end


=begin
== ((:OptionParser::List:))
Simple option list providing mapping from short and/or long option
string to ((<OptionParser::Switch>)), and mapping from acceptable
argument to matching pattern and converter pair.  Also provides
summary feature.
=end #'#"#`#
  class List
=begin
=== Class methods
=end #'#"#`#
=begin private
--- OptionParser::List.new
    Just initializes all instance variables.
=end #'#"#`#
    def initialize
      @atype = {}
      @short = OptionMap.new
      @long = OptionMap.new
      @list = []
    end

=begin
=== Instance methods
=end #'#"#`#
=begin
--- OptionParser::List#atype
    Map from acceptable argument types to pattern and converter pairs.
--- OptionParser::List#short
    Map from short style option switches to actual switch objects.
--- OptionParser::List#long
    Map from long style option switches to actual switch objects.
--- OptionParser::List#list
    List of all switches and summary string.
=end #'#"#`#
    attr_reader :atype, :short, :long, :list

=begin private
--- OptionParser::List#accept(type[, pattern]) {...}
    see ((<OptionParser.accept>)).
--- OptionParser::List#reject(type)
    see ((<OptionParser.reject>)).
=end #'#"#`#
    def accept(t, pat = nil, &block)
      if pat
	pat.respond_to?(:match) or raise TypeError, "has no `match'"
      else
	pat = t if t.respond_to?(:match)
      end
      unless block
	block = pat.method(:convert).to_proc if pat.respond_to?(:convert)
      end
      @atype[t] = [pat, block]
    end

    def reject(t)
      @atype.delete(t)
    end

=begin private
--- OptionParser::List#update(sw, sopts, lopts, nlopts = nil)
    Adds ((|sw|)) according to ((|sopts|)), ((|lopts|)) and
    ((|nlopts|)).
    :Parameters:
      : ((|sw|))
        ((<OptionParser::Switch>)) instance to be added.
      : ((|sopts|))
        short style options list.
      : ((|lopts|))
        long style options list.
      : ((|nlopts|))
        negated long style options list.
=end #'#"#`#
    def update(sw, sopts, lopts, nsw = nil, nlopts = nil)
      o = nil
      sopts.each {|o| @short[o] = sw} if sopts
      lopts.each {|o| @long[o] = sw} if lopts
      nlopts.each {|o| @long[o] = nsw} if nsw and nlopts
      used = @short.invert.update(@long.invert)
      @list.delete_if {|o| Switch === o and !used[o]}
    end
    private :update

=begin
--- OptionParser::List#prepend(switch, short_opts, long_opts, nolong_opts)
    Inserts ((|switch|)) at head of the list, and associates short,
    long and negated long options.
--- OptionParser::List#append(switch, short_opts, long_opts, nolong_opts)
    Appends ((|switch|)) at tail of the list, and associates short,
    long and negated long options.
    :Parameters:
      : ((|switch|))
        ((<OptionParser::Switch>)) instance to be inserted.
      : ((|short_opts|))
        list of short style options.
      : ((|long_opts|))
        list of long style options.
      : ((|nolong_opts|))
        list of long style options with (({"no-"})) prefix.
=end #'#"#`#
    def prepend(*args)
      update(*args)
      @list.unshift(args[0])
    end

    def append(*args)
      update(*args)
      @list.push(args[0])
    end

=begin
--- OptionParser::List#search(id, key) [{block}]
    Searches ((|key|)) in ((|id|)) list.
    :Parameters:
      : ((|id|))
        searching list.
      : ((|k|))
        searching key.
      : (({Block}))
        yielded with the found value when succeeded.
=end #'#"#`#
    def search(id, key)
      if list = __send__(id)
	val = list.fetch(key) {return nil}
	return val unless block_given?
	yield(val)
      end
    end

=begin
--- OptionParser::List#complete(id, opt, *pat, &block)
    Searches list ((|id|)) for ((|opt|)) and ((|*pat|)).
    :Parameters:
      : ((|id|))
        searching list.
      : ((|opt|))
        searching key.
      : ((|*pat|))
        optional pattern for completion.
      : (({Block}))
        yielded with the found value when succeeded.
=end #'#"#`#
    def complete(id, opt, *pat, &block)
      __send__(id).complete(opt, *pat, &block)
    end

=begin
--- OptionParser::List#summarize(*args) {...}
    Making summary table, yields the (({block})) with each lines.
    Each elements of (({@list})) should be able to (({summarize})).
    :Parameters:
      : ((|args|))
        passed to elements#summarize through.
      : (({block}))
        to be passed each lines(without newline).
=end #'#"#`#
    def summarize(*args, &block)
      list.each do |opt|
	if opt.respond_to?(:summarize) # perhaps OptionParser::Switch
	  opt.summarize(*args, &block)
	elsif opt.empty?
	  yield("")
	else
	  opt.each(&block)
	end
      end
    end
  end


=begin private
== ((:OptionParser::CompletingHash:))
(({Hash})) with completion search feature.
=== Superclass
(({Hash}))
=== Including modules
((<OptionParser::Completion>))
=end #'#"#`#
  class CompletingHash < Hash
    include Completion

=begin private
=== Instance methods
--- OptionParser::CompletingHash#match(key)
    Completion for hash key.
=end #'#"#`#
    def match(key)
      return key, *fetch(key) {
	raise AmbiguousArgument, catch(:ambiguous) {return complete(key)}
      }.to_a
    end
  end


=begin
== ((:OptionParser:))
The front-end of (({OptionParser})).
=end #'#"#`#

=begin
=== Constants
=end #'#"#`#

=begin
--- OptionParser::ArgumentStyle
    Enumeration of acceptable argument styles; possible values are:
    : OptionParser::NO_ARGUMENT
      the switch takes no arguments. ((({:NONE}))) 
    : OptionParser::REQUIRED_ARGUMENT
      the switch requires an argument. ((({:REQUIRED})))
    : OptionParser::OPTIONAL_ARGUMENT
      the switch requires an optional argument, that is, may take or
      not. ((({:OPTIONAL})))

    Use like (({--switch=argument}))(long style) or
    (({-Xargument}))(short style). For short style, only portion
    matched to ((<argument pattern>)) is dealed as argument.
=end #'#"#`#
  ArgumentStyle = {}
  NoArgument.each {|el| ArgumentStyle[el] = Switch::NoArgument}
  RequiredArgument.each {|el| ArgumentStyle[el] = Switch::RequiredArgument}
  OptionalArgument.each {|el| ArgumentStyle[el] = Switch::OptionalArgument}
  ArgumentStyle.freeze

=begin private
--- OptionParser::DefaultList
    Switches common used such as '--', and also provides default
    argument classes
=end #'#"#`#
  DefaultList = List.new
  DefaultList.short['-'] = Switch::NoArgument.new {}
  DefaultList.long[''] = Switch::NoArgument.new {throw :terminate}

=begin undocumented
=== Default options
Default options, which never appear in option summary.
--- --help
    Shows option summary.
--- --version
    Shows version string if (({::Version})) is defined.
=end #'#"#`#
  DefaultList.long['help'] = Switch::NoArgument.new do
    puts ARGV.options
    exit
  end
  DefaultList.long['version'] = Switch::NoArgument.new do
    if v = ARGV.options.ver
      puts v
      exit
    end
  end

=begin
=== Class methods
=end #'#"#`#

=begin
--- OptionParser.with([banner[, width[, indent]]]) [{...}]
    Initializes new instance, and evaluates the block in context of
    the instance if called as iterator.  This behavior is equivalent
    to older (({new})).  This is ((*deprecated*)) method.
    
    cf. ((<OptionParser.new>))
    :Parameters:
      : ((|banner|))
        banner message.
      : ((|width|))
        summary width.
      : ((|indent|))
        summary indent.
      : (({Block}))
        to be evaluated in the new instance context.
=end #'#"#`#
  def self.with(*args, &block)
    opts = new(*args)
    opts.instance_eval(&block)
    opts
  end

=begin
--- OptionParser.inc(arg[, default])
--- OptionParser#inc(arg[, default])
    Returns incremented value of ((|default|)) according to ((|arg|)).
=end
  def self.inc(arg, default = nil)
    case arg
    when Integer
      arg.nonzero?
    when nil
      default.to_i + 1
    end
  end
  def inc(*args)
    self.class.inc(*args)
  end

=begin
--- OptionParser.new([banner[, width[, indent]]]) [{...}]
    Initializes the instance, and yields itself if called as iterator.
    :Parameters:
      : ((|banner|))
        banner message.
      : ((|width|))
        summary width.
      : ((|indent|))
        summary indent.
      : (({Block}))
        to be evaluated in the new instance context.
=end #'#"#`#
  def initialize(banner = nil, width = 32, indent = ' ' * 4)
    @stack = [DefaultList, List.new, List.new]
    @program_name = nil
    @banner = banner
    @summary_width = width
    @summary_indent = indent
    yield self if block_given?
  end

=begin
--- OptionParser.terminate([arg])
    Terminates option parsing. Optional parameter ((|arg|)) would be
    pushed back if given.
    :Parameters:
      : ((|arg|))
        string pushed back to be first non-option argument
=end #'#"#`#
  def terminate(arg = nil)
    self.class.terminate(arg)
  end
  def self.terminate(arg = nil)
    throw :terminate, arg
  end

  @stack = [DefaultList]
  def self.top() DefaultList end

=begin
--- OptionParser.accept(t, [pat]) {...}
--- OptionParser#accept(t, [pat]) {...}
    Directs to accept specified class argument.
    :Parameters:
      : ((|t|))
        argument class specifier, any object including Class.
      : ((|pat|))
        pattern for argument, defaulted to ((|t|)) if it respond to (({match})).
      : (({Block}))
        receives argument string and should be convert to desired class.
=end #'#"#`#
  def accept(*args, &blk) top.accept(*args, &blk) end
  def self.accept(*args, &blk) top.accept(*args, &blk) end

=begin
--- OptionParser.reject(t)
--- OptionParser#reject(t)
    Directs to reject specified class argument.
    :Parameters:
      : ((|t|))
        argument class specifier, any object including Class.
=end #'#"#`#
  def reject(*args, &blk) top.reject(*args, &blk) end
  def self.reject(*args, &blk) top.reject(*args, &blk) end


=begin
=== Instance methods
=end #'#"#`#

=begin
--- OptionParser#banner
--- OptionParser#banner=(heading)
    Heading banner preceding summary.
--- OptionParser#summary_width
--- OptionParser#summary_width=(width)
    Width for option list portion of summary. Must be (({Numeric})).
--- OptionParser#summary_indent
--- OptionParser#summary_indent=(indent)
    Indentation for summary. Must be (({String})) (or have (({+ String}))).
--- OptionParser#program_name
--- OptionParser#program_name=(name)
    Program name to be emitted in error message and default banner,
    defaulted to (({$0})).
=end #'#"#`#
  attr_writer :banner, :program_name
  attr_accessor :summary_width, :summary_indent

  def banner
    @banner ||= "Usage: #{program_name} [options]"
  end

  def program_name
    @program_name || File.basename($0, '.*')
  end

# for experimental cascading :-)
  alias set_banner banner=
  alias set_program_name program_name=
  alias set_summary_width summary_width=
  alias set_summary_indent summary_indent=

=begin
--- OptionParser#version
--- OptionParser#version=(ver)
    Version.
--- OptionParser#release
--- OptionParser#release=(rel)
    Release code.
--- OptionParser#ver
    Returns version string from ((<program_name>)), (({version})) and
    (({release})).
=end #'#"#`#
  attr_writer :version, :release

  def version
    @version || (defined?(::Version) && ::Version)
  end

  def release
    @release || (defined?(::Release) && ::Release)
  end

  def ver
    if v = version
      str = "#{program_name} #{[v].join('.')}"
      str << " (#{v})" if v = release
      str
    end
  end

=begin
--- OptionParser#top
    Subject of ((<on>))/((<on_head>)), ((<accept>))/((<reject>)).
=end #'#"#`#
  def top
    @stack[-1]
  end

=begin
--- OptionParser#base
    Subject of ((<on_tail>)).
=end #'#"#`#
  def base
    @stack[1]
  end

=begin
--- OptionParser#new
    Pushes a new (({List})).
=end #'#"#`#
  def new
    @stack.push(List.new)
    if block_given?
      yield self
    else
      self
    end
  end

=begin
--- OptionParser#remove
    Removes the last (({List})).
=end #'#"#`#
  def remove
    @stack.pop
  end


=begin
--- OptionParser#summarize(to = [], width = @summary_width, max = width - 1, indent = @summary_indent)
    Puts option summary into ((|to|)), and returns ((|to|)).
    :Parameters:
      : ((|to|))
        output destination, which must have method ((|<<|)). Defaulted to (({[]})).
      : ((|width|))
        width of left side. Defaulted to ((|@summary_width|))
      : ((|max|))
        maximum length allowed for left side. Defaulted to (({((|width|)) - 1}))
      : ((|indent|))
        indentation. Defaulted to ((|@summary_indent|))
      : (({Block}))
        yields with each line if called as iterator.
=end #'#"#`#
  def summarize(to = [], width = @summary_width, max = width - 1, indent = @summary_indent, &blk)
    visit(:summarize, {}, {}, width, max, indent, &(blk || proc {|l| to << l + $/}))
    to
  end

=begin
--- OptionParser#to_str
--- OptionParser#to_s
    Returns option summary string.
=end #'#"#`#
  def to_str; summarize(banner.to_s.sub(/\n?\z/, "\n")) end
  alias to_s to_str

=begin
--- OptionParser#to_a
    Returns option summary list.
=end #'#"#`#
  def to_a; summarize(banner.to_a.dup) end


=begin
--- OptionParser#switch
    Creates ((<OptionParser::Switch>)).
    :Parameters:
      : ((|*opts|))
        option definition:
        : argument style
          see ((<OptionParser::ArgumentStyle>))
        : argument pattern
          acceptable option argument format, must pre-defined with
          ((<OptionParser.accept>)) or ((<OptionParser#accept>)), or
          (({Regexp})). This can appear once or assigned as (({String}))
          if not present, otherwise causes exception (({ArgumentError})).
          
          cf. ((<Acceptable argument classes>)).
        : Hash
        : Array
          possible argument values.
        : Proc
        : Method
          alternative way to give the ((*handler*)).
        : "--switch=MANDATORY", "--switch[=OPTIONAL]", "--switch"
          specifies long style switch that takes ((*mandatory*)),
          ((*optional*)) and ((*no*)) argument, respectively.
        : "-xMANDATORY", "-x[OPTIONAL]", "-x"
          specifies short style switch that takes ((*mandatory*)),
          ((*optional*)) and ((*no*)) argument, respectively.
        : "-[a-z]MANDATORY", "-[a-z][OPTIONAL]", "-[a-z]"
          special form short style switch that matches character
          range(not fullset of regular expression).
        : "=MANDATORY", "=[OPTIONAL]"
          argument style and description.
        : "description", ...
          ((*description*)) for this option.
      : (({Block}))
        ((*handler*)) to convert option argument to arbitrary (({Class})).
=end #'#"#`#
=begin private
--- OptionParser#notwice(obj, prv, msg)
    Checks never given twice an argument.
    ((*Called from OptionParser#switch only*))
    :Parameters:
      : ((|obj|))
        new argument.
      : ((|prv|))
        previously specified argument.
      : ((|msg|))
        exception message
=end #'#"#`#
  def notwice(obj, prv, msg)
    unless !prv or prv == obj
      begin
	raise ArgumentError, "argument #{msg} given twice: #{obj}"
      rescue
	$@[0, 2] = nil
	raise
      end
    end
    obj
  end
  private :notwice

  def switch(*opts, &block)
    short, long, nolong, style, pattern, conv, not_pattern, not_conv, not_style = [], [], []
    ldesc, sdesc, desc, arg = [], [], []
    default_style = Switch::NoArgument
    default_pattern = nil
    klass = nil
    o = nil
    n, q, a = nil

    opts.each do |o|
      # argument class
      next if search(:atype, o) do |pat, c|
	klass = notwice(o, klass, 'type')
	if not_style and not_style != Switch::NoArgument
	  not_pattern, not_conv = pat, c
	else
	  default_pattern, conv = pat, c
	end
      end

      # directly specified pattern(any object possible to match)
      if !(String === o) and o.respond_to?(:match)
	pattern = notwice(o, pattern, 'pattern')
	conv = (pattern.method(:convert).to_proc if pattern.respond_to?(:convert))
	next
      end

      # anything others
      case o
      when Proc, Method
	block = notwice(o, block, 'block')
      when Array, Hash
	case pattern
	when CompletingHash
	when nil
	  pattern = CompletingHash.new
	  conv = (pattern.method(:convert).to_proc if pattern.respond_to?(:convert))
	else
	  raise ArgumentError, "argument pattern given twice"
	end
	if Array === o
	  o.each {|o| pattern[(Array === o ? o.shift : o)] = o}
	else
	  pattern.update(o)
	end
      when Module
	raise ArgumentError, "unsupported argument type: #{o}"
      when *ArgumentStyle.keys
	style = notwice(ArgumentStyle[o], style, 'style')
      when /^--no-([^][=\s]*)(.+)?/
	q, a = $1, $2
	o = notwice(a ? Object : TrueClass, klass, 'type')
	not_pattern, not_conv = search(:atype, o) unless not_style
	not_style = (not_style || default_style).guess(arg = a) if a
	default_style = Switch::NoArgument
	default_pattern, conv = search(:atype, FalseClass)
	ldesc << "--no-#{q}"
	long << 'no-' + (q = q.downcase)
	nolong << q
      when /^--\[no-\]([^][=\s]*)(.+)?/
	q, a = $1, $2
	o = notwice(a ? Object : TrueClass, klass, 'type')
	default_style = default_style.guess(arg = a) if a
	default_pattern, conv = search(:atype, o)
	ldesc << "--#{q}"
	long << (o = q.downcase)
	not_pattern, not_conv = search(:atype, FalseClass) unless not_style
	not_style = Switch::NoArgument
	nolong << 'no-' + o
      when /^--([^][=\s]*)(.+)?/
	q, a = $1, $2
	o = notwice(a ? NilClass : TrueClass, klass, 'type')
	default_style = default_style.guess(arg = a) if a
	default_pattern, conv = search(:atype, o)
	ldesc << "--#{q}"
	long << (o = q.downcase)
      when /^-(\[\^?\]?(?:[^\\\]]|\\.)*\])(.+)?/
	q, a = $1, $2
	o = notwice(Object, klass, 'type')
	default_style = default_style.guess(arg = a) if a
	default_pattern, conv = search(:atype, o)
	sdesc << "-#{q}"
	short << Regexp.new(q)
      when /^-(.)(.+)?/
	q, a = $1, $2
	o = notwice((a ? Object : TrueClass), klass, 'type')
	default_style = default_style.guess(arg = a) if a
	default_pattern, conv = search(:atype, o)
	sdesc << "-#{q}"
	short << q
      when /^=/
	style = notwice(default_style.guess(arg = o), style, 'style')
	default_pattern, conv = search(:atype, Object)
      else
	desc.push(o)
      end
    end

    s = if short.empty? and long.empty?
	  raise ArgumentError, "no switch given" if style or pattern or block
	  desc
	else
	  (style || default_style).new(pattern || default_pattern,
				       conv, sdesc, ldesc, arg, desc, block)
	end
    return s, short, long,
      (not_style.new(not_pattern, not_conv, sdesc, ldesc, nil, desc, block) if not_style),
      nolong
  end

=begin
--- OptionParser#on(*opts) [{...}]
--- OptionParser#def_option(*opts) [{...}]
--- OptionParser#on_head(*opts) [{...}]
--- OptionParser#def_head_option(*opts) [{...}]
--- OptionParser#on_tail(*opts) [{...}]
--- OptionParser#def_tail_option(*opts) [{...}]
    Defines option switch and handler. (({on_head})), (({def_head_option}))
    and (({on_tail})), (({def_tail_option})) put the switch at head
    and tail of summary, respectively.

    cf. ((<OptionParser#switch>)).
=end #'#"#`#
  def on(*opts, &block)
    top.append(*switch(*opts, &block))
    self
  end
  alias def_option on

  def on_head(*opts, &block)
    top.prepend(*switch(*opts, &block))
    self
  end
  alias def_head_option on_head

  def on_tail(*opts, &block)
    base.append(*switch(*opts, &block))
    self
  end
  alias def_tail_option on_tail


=begin
--- OptionParser#order(*argv) [{...}]
--- OptionParser#order!([argv = ARGV]) [{...}]
    Parses ((|argv|)) in order. When non-option argument encountered,
    yields it if called as iterator, otherwise terminates the parse
    process.
    Returns rest of ((|argv|)) left unparsed.
    
    (({order!})) takes argument array itself, and removes switches
    destructively.
    Defaults to parse ((|ARGV|)).
    :Parameters:
      : ((|argv|))
        command line arguments to be parsed.
      : (({Block}))
        called with each non-option argument.
=end #'#"#`#
  def order(*argv, &block) order!(argv, &block) end

  def order!(argv = ARGV, &nonopt)
    opt, arg, sw, val, rest = nil
    nonopt ||= proc {|arg| throw :terminate, arg}
    argv.unshift(arg) if arg = catch(:terminate) {
      while arg = argv.shift
	case arg
	# long option
	when /\A--([^=]*)(?:=(.*))?/
	  opt, rest = $1, $2
	  begin
	    sw, = complete(:long, opt)
	  rescue ParseError
	    raise $!.set_option(arg, true)
	  end
	  begin
	    opt, sw, val = sw.parse(rest, argv) {|*exc| raise(*exc)}
	    sw.yield(val) if sw
	  rescue ParseError
	    raise $!.set_option(arg, rest)
	  end

	# short option
	when /\A-(.)((=).*|.+)?/
	  opt, has_arg, eq, val, rest = $1, $3, $3, $2, $2
	  begin
	    unless sw = search(:short, opt)
	      begin
		sw, = complete(:short, opt)
		# short option matched.
		val = arg.sub(/\A-/, '')
		has_arg = true
	      rescue InvalidOption
		# if no short options match, try completion with long
		# options.
		sw, = complete(:long, opt)
		eq ||= !rest
	      end
	    end
	  rescue ParseError
	    raise $!.set_option(arg, true)
	  end
	  begin
	    opt, sw, val = sw.parse(val, argv) {|*exc| raise(*exc) if eq}
	    raise InvalidOption, arg if has_arg and !eq and arg == "-#{opt}"
	    argv.unshift(opt) if opt and (opt = opt.sub(/\A-*/, '-')) != '-'
	    sw.yield(val) if sw
	  rescue ParseError
	    raise $!.set_option(arg, has_arg)
	  end

	# non-option argument
	else
	  nonopt.call(arg)
	end
      end

      nil
    }

    argv
  end

=begin
--- OptionParser#permute(*argv)
--- OptionParser#permute!([argv = ARGV])
    Parses ((|argv|)) in permutation mode, and returns list of
    non-option arguments.
    
    (({permute!})) takes argument array itself, and removes switches
    destructively.
    Defaults to parse ((|ARGV|)).
    :Parameters:
      : ((|argv|))
        command line arguments to be parsed.
=end #'#"#`#
  def permute(*argv)
    permute!(argv)
  end

  def permute!(argv = ARGV)
    nonopts = []
    arg = nil
    order!(argv) {|arg| nonopts << arg}
    argv[0, 0] = nonopts
    argv
  end

=begin
--- OptionParser#parse(*argv)
--- OptionParser#parse!([argv = ARGV])
    Parses ((|argv|)) in order when environment variable (({POSIXLY_CORRECT}))
    is set, otherwise permutation mode
    
    (({parse!})) takes argument array itself, and removes switches
    destructively.
    Defaults to parse ((|ARGV|)).
    :Parameters:
      : ((|argv|))
        command line arguments to be parsed.
=end #'#"#`#
  def parse(*argv)
    parse!(argv)
  end

  def parse!(argv = ARGV)
    if ENV.include?('POSIXLY_CORRECT')
      order!(argv)
    else
      permute!(argv)
    end
  end


=begin private
--- OptionParser#visit(id, *args) {block}
    Traverses (({stack}))s calling method ((|id|)) with ((|*args|)).
    :Parameters:
      : ((|id|))
        called method in each elements of (({stack}))s.
      : ((|*args|))
        passed to ((|id|)).
      : (({Block}))
        passed to ((|id|)).
=end #'#"#`#
  def visit(id, *args, &block)
    el = nil
    @stack.reverse_each do |el|
      el.send(id, *args, &block)
    end
    nil
  end
  private :visit

=begin private
--- OptionParser#search(id, k)
    Searches ((|k|)) in stack for ((|id|)) hash, and returns it or yielded
    value if called as iterator.
    :Parameters:
      : ((|id|))
        searching table.
      : ((|k|))
        searching key.
      : (({Block}))
        yielded with the found value when succeeded.
=end #'#"#`#
  def search(id, k)
    visit(:search, id, k) do |k|
      return k unless block_given?
      return yield(k)
    end
  end
  private :search

=begin private
--- OptionParser#complete(typ, opt, *etc)
    Completes shortened long style option switch, and returns pair of
    canonical switch and switch descriptor((<OptionParser::Switch>)).
    :Parameters:
      : ((|id|))
        searching table.
      : ((|opt|))
        searching key.
      : ((|*pat|))
        optional pattern for completion.
      : (({Block}))
        yielded with the found value when succeeded.
=end #'#"#`#
  def complete(typ, opt, *pat)
    if pat.empty?
      search(typ, opt) {|sw| return [sw, opt]} # exact match or...
    end
    raise AmbiguousOption, catch(:ambiguous) {
      visit(:complete, typ, opt, *pat) {|opt, *sw| return sw}
      raise InvalidOption, opt
    }
  end
  private :complete

=begin undocumented
--- OptionParser#load([filename])
    Loads options from file named as ((|filename|)). Does nothing when
    the file is not present. Returns whether successfuly loaded.
    :Parameters:
      : ((|filename|))
        option file name.  defaulted to basename of the program without
        suffix in a directory ((%~/.options%)).
=end #'#"#`#
  def load(filename = nil)
    begin
      filename ||= File.expand_path(File.basename($0, '.*'), '~/.options')
    rescue
      return false
    end
    begin
      parse(*IO.readlines(filename).each {|s| s.chomp!})
      true
    rescue Errno::ENOENT, Errno::ENOTDIR
      false
    end
  end

=begin undocumented
--- OptionParser#environment([env])
    Parses environment variable ((|env|)) or its uppercase with spliting
    like as shell.
    :Parameters:
      : ((|env|))
        defaulted to basename of the program.
=end #'#"#`#
  def environment(env = File.basename($0, '.*'))
    env = ENV[env] || ENV[env.upcase] or return
    parse(*Shellwords.shellwords(env))
  end


=begin
= Acceptable argument classes
=end #'#"#`#

=begin
: Object
  any string, and no conversion. this is fall-back.
=end #'#"#`#
  accept(Object) {|s|s or s.nil?}

  accept(NilClass) {|s|s}

=begin
: String
  any none-empty string, and no conversion.
=end #'#"#`#
  accept(String, /.+/) {|s,*|s}

=begin
: Integer
  Ruby/C-like integer, octal for (({0-7})) sequence, binary for
  (({0b})), hexadecimal for (({0x})), and decimal for others; with
  optional sign prefix. Converts to (({Integer})).
=end #'#"#`#
  decimal = '\d+(?:_\d+)*'
  binary = 'b[01]+(?:_[01]+)*'
  hex = 'x[\da-f]+(?:_[\da-f]+)*'
  octal = "0(?:[0-7]*(?:_[0-7]+)*|#{binary}|#{hex})"
  integer = "#{octal}|#{decimal}"
  accept(Integer, %r"\A[-+]?(?:#{integer})"io) {|s| Integer(s) if s}

=begin
: Float
  Float number format, and converts to (({Float})).
=end #'#"#`#
  float = "(?:#{decimal}(?:\\.(?:#{decimal})?)?|\\.#{decimal})(?:E[-+]?#{decimal})?"
  floatpat = %r"\A[-+]?#{float}"io
  accept(Float, floatpat) {|s| s.to_f if s}

=begin
: Numeric
  Generic numeric format, and converts to (({Integer})) for integer
  format, (({Float})) for float format.
=end #'#"#`#
  accept(Numeric, %r"\A[-+]?(?:#{octal}|#{float})"io) {|s| eval(s) if s}

=begin
: OptionParser::DecimalInteger
  Decimal integer format, to be converted to (({Integer})).
=end #'#"#`#
  DecimalInteger = /\A[-+]?#{decimal}/io
  accept(DecimalInteger) {|s| s.to_i if s}

=begin
: OptionParser::OctalInteger
  Ruby/C like octal/hexadecimal/binary integer format, to be converted
  to (({Integer})).
=end #'#"#`#
  OctalInteger = /\A[-+]?(?:[0-7]+(?:_[0-7]+)*|0(?:#{binary}|#{hex}))/io
  accept(OctalInteger) {|s| s.oct if s}

=begin
: OptionParser::DecimalNumeric
  Decimal integer/float number format, to be converted to
  (({Integer})) for integer format, (({Float})) for float format.
=end #'#"#`#
  DecimalNumeric = floatpat	# decimal integer is allowed as float also.
  accept(DecimalNumeric) {|s| eval(s) if s}

=begin
: TrueClass
  Boolean switch, which means whether it is present or not, whether it
  is absent or not with prefix (({no-})), or it takes an argument
  (({yes/no/true/false/+/-})).
: FalseClass
  Similar to ((<TrueClass>)), but defaulted to (({false})).
=end #'#"#`#
  yesno = CompletingHash.new
  %w[- no false].each {|el| yesno[el] = false}
  %w[+ yes true].each {|el| yesno[el] = true}
  yesno['nil'] = false		# shoud be nil?
  accept(TrueClass, yesno) {|arg, val| val == nil or val}
  accept(FalseClass, yesno) {|arg, val| val != nil and val}

=begin
: Array
  List of strings separated by ","
=end #'#"#`#
  accept(Array) do |s|
    if s
      s = s.split(',').collect {|s| s unless s.empty?}
      s.size > 1 or s = [s]	# guard empty or single.
    end
    s
  end


=begin
= Exceptions
=end #'#"#`#

=begin
== ((:OptionParser::ParseError:))
Base class of exceptions from ((<OptionParser>))
=== Superclass
(({RuntimeError}))
=== Constants
: OptionParser::ParseError::Reason
  Reason caused error.
=== Instance methods
--- OptionParser::ParseError#recover(argv)
    Push backs erred argument(s) to ((|argv|)).
--- OptionParser::ParseError#reason
    Returns error reason. Override this to I18N.
--- OptionParser::ParseError#inspect
    Returns inspection string.
--- OptionParser::ParseError#message
--- OptionParser::ParseError#to_s
--- OptionParser::ParseError#to_str
    Default stringizing method to emit standard error message.
=end #'#"#`#
  class ParseError < RuntimeError
    Reason = 'parse error'.freeze

    def initialize(*args)
      @args = args
      @reason = nil
    end

    attr_reader :args
    attr_writer :reason

    def recover(argv)
      argv[0, 0] = @args
      argv
    end

    def set_option(opt, eq)
      if eq
	@args[0] = opt
      else
	@args.unshift(opt)
      end
      self
    end

    def reason
      @reason || self.class::Reason
    end

    def inspect
      "#<#{self.class.to_s}: #{args.join(' ')}>"
    end

    def message
      reason + ': ' + args.join(' ')
    end

    alias to_s message
    alias to_str message
  end

=begin
== ((:OptionParser::AmbiguousOption:))
Raises when encountered ambiguously completable string.
=== Superclass
((<OptionParser::ParseError>))
=end #'#"#`#
  class AmbiguousOption < ParseError
    const_set(:Reason, 'ambiguous option'.freeze)
  end

=begin
== ((:OptionParser::NeedlessArgument:))
Raises when encountered argument for switch defined as which takes no
argument.
=== Superclass
((<OptionParser::ParseError>))
=end #'#"#`#
  class NeedlessArgument < ParseError
    const_set(:Reason, 'needles argument'.freeze)
  end

=begin
== ((:OptionParser::MissingArgument:))
Raises when no argument found for switch defined as which needs
argument.
=== Superclass
((<OptionParser::ParseError>))
=end #'#"#`#
  class MissingArgument < ParseError
    const_set(:Reason, 'missing argument'.freeze)
  end

=begin
== ((:OptionParser::InvalidOption:))
Raises when undefined switch.
=== Superclass
((<OptionParser::ParseError>))
=end #'#"#`#
  class InvalidOption < ParseError
    const_set(:Reason, 'invalid option'.freeze)
  end

=begin
== ((:OptionParser::InvalidArgument:))
Raises when the given argument does not match required format.
=== Superclass
((<OptionParser::ParseError>))
=end #'#"#`#
  class InvalidArgument < ParseError
    const_set(:Reason, 'invalid argument'.freeze)
  end

=begin
== ((:OptionParser::AmbiguousArgument:))
Raises when the given argument word can't completed uniquely.
=== Superclass
((<OptionParser::InvalidArgument>))
=end #'#"#`#
  class AmbiguousArgument < InvalidArgument
    const_set(:Reason, 'ambiguous argument'.freeze)
  end


=begin
= Miscellaneous
=end #'#"#`#
=begin
== ((:OptionParser::Arguable:))
Extends command line arguments array to parse itself.
=end #'#"#`#
  module Arguable
=begin
--- OptionParser::Arguable#options=(opt)
    Sets ((<OptionParser>)) object, when ((|opt|)) is (({false})) or
    (({nil})), methods ((<OptionParser::Arguable#options>)) and
    ((<OptionParser::Arguable#options=>)) are undefined.  Thus, there
    is no ways to access the ((<OptionParser>)) object via the
    receiver object.
=end #'#"#`#
    def options=(opt)
      unless @optparse = opt
	class << self
	  undef_method(:options)
	  undef_method(:options=)
	end
      end
    end

=begin
--- OptionParser::Arguable#options
    Actual ((<OptionParser>)) object, automatically created if not
    yet.

    If called as iterator, yields with the ((<OptionParser>)) object
    and returns the result of the block. In this case, rescues any
    ((<OptionParser::ParseError>)) exceptions in the block, just emits
    error message to ((<STDERR>)) and returns (({nil})).

    :Parameters:
      : (({block}))
        Yielded with the ((<OptionParser>)) instance.

=end #'#"#`#
    def options
      @optparse ||= OptionParser.new
      block_given? or return @optparse
      begin
	yield @optparse
      rescue ParseError
	STDERR.puts @optparse.program_name + ': ' + $!
	nil
      end
    end

=begin
--- OptionParser::Arguable#order!
--- OptionParser::Arguable#permute!
--- OptionParser::Arguable#parse!
    Parses ((|self|)) destructively, and returns ((|self|)) just contains
    rest arguments left without parsed.
=end #'#"#`#
    def order!(&blk) options.order!(self, &blk) end
    def permute!() options.permute!(self) end
    def parse!() options.parse!(self) end

=begin private
Initializes instance variable.
=end #'#"#`#
    def self.extend_object(obj)
      super
      obj.instance_eval {@optparse = nil}
    end
    def initialize(*args)
      super
      @optparse = nil
    end
  end

=begin
== OptionParser::Acceptables
Acceptable argument classes.  Now contains (({DecimalInteger})),
(({OctalInteger})) and (({DecimalNumeric})).
see ((<Acceptable argument classes>)).
=end #'#"#`#
  module Acceptables
    const_set(:DecimalInteger, OptionParser::DecimalInteger)
    const_set(:OctalInteger, OptionParser::OctalInteger)
    const_set(:DecimalNumeric, OptionParser::DecimalNumeric)
  end
end

# ARGV is arguable by OptionParser
ARGV.extend(OptionParser::Arguable)


if $0 == __FILE__
  Version = OptionParser::Version
  ARGV.options {|q|
    q.parse!.empty? or puts "what's #{ARGV.join(' ')}?"
  } or exit 1
end
__END__
=begin example
= Example
<<< opttest.rb
=end #'#"#`#
