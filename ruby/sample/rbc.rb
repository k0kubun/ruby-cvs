#!/usr/local/bin/ruby
#
#   rbc.rb - 
#   	$Release Version: 0.8 $
#   	$Revision$
#   	$Date$
#   	by Keiju ISHITSUKA(Nippon Rational Inc.)
#
# --
# Usage:
#
#   rbc.rb [options] file_name opts
#   options:
#	-d		    debug mode (not recommended)
#	-f		    does not read ~/.irbrc
#	-m		    bc mode (rational/matrix calc)
#	-r load-module	    same as `ruby -r'
#	--inspect	    use inspect for result output
#			    (default for non-bc mode)
#	--noinspect	    does not use inspect for result output
#	--noreadline	    does not use readline library
#			    (default: try to use readline)
#
# additional private method (as function):
#   exit, quit		    terminate the interpreter
#   inspect_mode(sw = nil)  toggle inspect mode
#   trace_load(sw = nil)    change trace mode for file loading using
#			    load/require.  (default: trace-mode on)
#
require "e2mmap.rb"

$stdout.sync = TRUE

module BC_APPLICATION__
  RCS_ID='-$Id$-'
  
  extend Exception2MessageMapper
  def_exception :UnrecognizedSwitch, "Unrecognized switch: %s"
  
  CONFIG = {}
  CONFIG[0] = $0
  CONFIG[:USE_READLINE] = TRUE
  CONFIG[:LOAD_MODULES] = []
  CONFIG[:INSPECT] = nil
  CONFIG[:TRACE_LOAD] = FALSE
  CONFIG[:RC] = TRUE

  CONFIG[:DEBUG] = FALSE

  while opt = ARGV.shift
    case opt
    when "-d"
      CONFIG[:DEBUG] = TRUE
    when "-m"
      CONFIG[:INSPECT] = FALSE if CONFIG[:INSPECT].nil?
      require "mathn.rb"
      include Math
    when "-r"
      opt = ARGV.shift
      CONFIG[:LOAD_MODULES].push opt if opt
    when "-f"
      opt = ARGV.shift
      CONFIG[:RC] = FALSE
    when "--inspect"
      CONFIG[:INSPECT] = TRUE
    when "--noinspect"
      CONFIG[:INSPECT] = FALSE
    when "--noreadline"
      CONFIG[:USE_READLINE] = FALSE
    when /^-/
      #	  print UnrecognizedSwitch.inspect, "\n"
      BC_APPLICATION__.fail UnrecognizedSwitch, opt
    else
      CONFIG[:USE_READLINE] = FALSE
      $0 = opt
      break
    end
  end
  CONFIG[:INSPECT] = TRUE if CONFIG[:INSPECT].nil?

  PROMPTi = "rbc%d> "
  PROMPTs = "rbc%d%s "
  PROMPTe = "rbc%d* "
  
  class BC
    def initialize
      lex_init
    end
    
    def eval_input(io, cont, bind)
      line = ''
      @io = io
      @ltype = nil
      @quoted = nil
      @indent = 0
      @lex_state = EXPR_BEG
      
      @io.prompt = format(PROMPTi, @indent)

      loop do
	@continue = FALSE
	l = @io.gets

	unless l
	  break if line == ''
	else
	  line = line + l
	  
	  lex(l) if l != "\n"
	  print @quoted.inspect, "\n" if CONFIG[:DEBUG]
	  if @ltype
	    @io.prompt = format(PROMPTs, @indent, @ltype)
	    next
	  elsif @continue
	    @io.prompt = format(PROMPTe, @indent)
	    next
	  elsif @indent > 0
	    @io.prompt = format(PROMPTi, @indent)
	    next
	  end
	end
	
	if line != "\n"
	  begin
	    if CONFIG[:INSPECT]
	      print((cont._=eval(line, bind)).inspect, "\n")
	    else
	      print((cont._=eval(line, bind)), "\n")
	    end
	  rescue StandardError, ScriptError
	    #	$! = 'exception raised' unless $!
	    #	print "ERR: ", $!, "\n"
	    $! = RuntimeError.new("exception raised") unless $!
	    print $!.type, ": ", $!, "\n"
	  end
	end
	break if not l
	line = ''
	indent = 0
	@io.prompt = format(PROMPTi, indent)
      end
      print "\n"
    end
    
    EXPR_BEG = :EXPR_BEG
    EXPR_MID = :EXPR_MID
    EXPR_END = :EXPR_END
    EXPR_ARG = :EXPR_ARG
    EXPR_FNAME = :EXPR_FNAME

    CLAUSE_STATE_TRANS = {
      "alias"	=>  EXPR_FNAME,
      "and"	=>  EXPR_BEG,
      "begin"	=>  EXPR_BEG,
      "case"	=>  EXPR_BEG,
      "class"	=>  EXPR_BEG,
      "def"	=>  EXPR_FNAME,
      "defined?"	=>  EXPR_END,
      "do"	=>  EXPR_BEG,
      "else"	=>  EXPR_BEG,
      "elsif"	=>  EXPR_BEG,
      "end"	=>  EXPR_END,
      "ensure"	=>  EXPR_BEG,
      "for"	=>  EXPR_BEG,
      "if"	=>  EXPR_BEG,
      "in"	=>  EXPR_BEG,
      "module"	=>  EXPR_BEG,
      "nil"	=>  EXPR_END,
      "not"	=>  EXPR_BEG,
      "or"	=>  EXPR_BEG,
      "rescue"	=>  EXPR_MID,
      "return"	=>  EXPR_MID,
      "self"	=>  EXPR_END,
      "super"	=>  EXPR_END,
      "then"	=>  EXPR_BEG,
      "undef"	=>  EXPR_FNAME,
      "unless"	=>  EXPR_BEG,
      "until"	=>  EXPR_BEG,
      "when"	=>  EXPR_BEG,
      "while"	=>  EXPR_BEG,
      "yield"	=>  EXPR_END
    }
    
    ENINDENT_CLAUSE = [
      "case", "class", "def", "do", "for", "if",
      "module", "unless", "until", "while", "begin" #, "when"
    ]
    DEINDENT_CLAUSE = ["end" #, "when"
    ]

    PARCENT_LTYPE = {
      "q" => "\'",
      "Q" => "\"",
      "x" => "\`",
      "r" => "\/"
    }
    
    PARCENT_PAREN = {
      "{" => "}",
      "[" => "]",
      "<" => ">",
      "(" => ")"
    }

    def lex_init()
      @OP = Trie.new
      @OP.def_rules("\0", "\004", "\032"){}
      @OP.def_rules(" ", "\t", "\f", "\r", "\13") do
	@space_seen = TRUE
	next
      end
      @OP.def_rule("#") do
	|op, rests|
	@ltype = "#"
	identify_comment(rests)
      end
      @OP.def_rule("\n") do
	print "\\n\n" if CONFIG[:DEBUG]
	if @lex_state == EXPR_BEG || @lex_state == EXPR_FNAME
	  @continue = TRUE
	else
	  @lex_state = EXPR_BEG
	end
      end
      @OP.def_rules("*", "*=", "**=", "**") {@lex_state = EXPR_BEG}
      @OP.def_rules("!", "!=", "!~") {@lex_state = EXPR_BEG}
      @OP.def_rules("=", "==", "===", "=~", "<=>") {@lex_state = EXPR_BEG}
      @OP.def_rules("<", "<=", "<<") {@lex_state = EXPR_BEG}
      @OP.def_rules(">", ">=", ">>") {@lex_state = EXPR_BEG}
      @OP.def_rules("'", '"') do
	|op, rests|
	@ltype = op
	@quoted = op
	identify_string(rests)
      end
      @OP.def_rules("`") do
	|op, rests|
	if @lex_state != EXPR_FNAME
	  @ltype = op
	  @quoted = op
	  identify_string(rests)
	end
      end
      @OP.def_rules('?') do
	|op, rests|
	@lex_state = EXPR_END
	identify_question(rests)
      end
      @OP.def_rules("&", "&&", "&=", "|", "||", "|=") do
	@lex_state = EXPR_BEG
      end
      @OP.def_rule("+@", proc{@lex_state == EXPR_FNAME}) {}
      @OP.def_rule("-@", proc{@lex_state == EXPR_FNAME}) {}
      @OP.def_rules("+=", "-=") {@lex_state = EXPR_BEG}
      @OP.def_rules("+", "-") do
	|op, rests|
	if @lex_state == EXPR_ARG
	  if @space_seen and rests[0] =~ /[0-9]/
	    identify_number(rests)
	  else
	    @lex_state = EXPR_BEG
	  end
	elsif @lex_state != EXPR_END and rests[0] =~ /[0-9]/
	  identify_number(rests)
	else
	  @lex_state = EXPR_BEG
	end
      end
      @OP.def_rule(".") do
	|op, rests|
	@lex_state = EXPR_BEG
	if rests[0] =~ /[0-9]/
	  rests.unshift op
	  identify_number(rests)
	else
	  # handle ``obj.if'' and such
	  identify_identifier(rests, TRUE)
	  @lex_state = EXPR_ARG
	end
      end
      @OP.def_rules("..", "...") {@lex_state = EXPR_BEG}

      lex_int2
    end
    
    def lex_int2
      @OP.def_rules("]", "}", ")") do
	@lex_state = EXPR_END
	@indent -= 1
      end
      @OP.def_rule(":") {|op,rests|
	identify_identifier(rests, TRUE)
      }
      @OP.def_rule("::") {|op,rests|
	identify_identifier(rests, TRUE);
      }
      @OP.def_rule("/") do
	|op, rests|
	if @lex_state == EXPR_BEG || @lex_state == EXPR_MID
	  @ltype = op
	  @quoted = op
	  identify_string(rests)
	elsif rests[0] == '='
	  rests.shift
	  @lex_state = EXPR_BEG
	elsif @lex_state == EXPR_ARG and @space_seen and rests[0] =~ /\s/
	  @ltype = op
	  @quoted = op
	  identify_string(rests)
	else 
	  @lex_state = EXPR_BEG
	end
      end
      @OP.def_rules("^", "^=") {@lex_state = EXPR_BEG}
      @OP.def_rules(",", ";") {@lex_state = EXPR_BEG}
      @OP.def_rule("~") {@lex_state = EXPR_BEG}
      @OP.def_rule("~@", proc{@lex_state = EXPR_FNAME}) {}
      @OP.def_rule("(") do
	@lex_state = EXPR_BEG
	@indent += 1
      end
      @OP.def_rule("[]", proc{@lex_state == EXPR_FNAME}) {}
      @OP.def_rule("[]=", proc{@lex_state == EXPR_FNAME}) {}
      @OP.def_rule("[") do
	@indent += 1
	if @lex_state != EXPR_FNAME
	  @lex_state = EXPR_BEG
	end
      end
      @OP.def_rule("{") do
	@lex_state = EXPR_BEG
	@indent += 1
      end
      @OP.def_rule('\\') {|op, rests| identify_escape(rests)} #')
      @OP.def_rule('%') do
	|op, rests|
	if @lex_state == EXPR_BEG || @lex_state == EXPR_MID
	  identify_quotation(rests)
	elsif rests[0] == '='
	  rests.shift
	elsif @lex_state == EXPR_ARG and @space_seen and rests[0] =~ /\s/
	  identify_quotation(rests)
	else
	  @lex_state = EXPR_BEG
	end
      end
      @OP.def_rule('$') do	#'
	|op, rests|
	identify_gvar(rests)
      end
      @OP.def_rule('@') do
	|op, rests|
	if rests[0] =~ /[\w_]/
	  rests.unshift op
	  identify_identifier(rests)
	end
      end
      @OP.def_rule("def", proc{|op, chrs| /\s/ =~ chrs[0]}) do 
	|op, rests|
	@indent += 1
	@lex_state = EXPR_END
	until rests[0] == "\n" or rests[0] == ";"
	  rests.shift
	end
      end
      @OP.def_rule("") do
	|op, rests|
	printf "MATCH: start %s: %s\n", op, rests.inspect if CONFIG[:DEBUG]
	if rests[0] =~ /[0-9]/
	  identify_number(rests)
	elsif rests[0] =~ /[\w_]/
	  identify_identifier(rests)
	end
	printf "MATCH: end %s: %s\n", op, rests.inspect if CONFIG[:DEBUG]
      end
      
      p @OP if CONFIG[:DEBUG]
    end
    
    def lex(l)
      chrs = l.split(//)
      tokens = []
      
      case @ltype
      when "'", '"', '`', '/'
	identify_string(chrs)
	return if chrs.empty?
      when "#"
	identify_comment(chrs)
	return
      when "="
	if l =~ /^=end/
	  $ltype = nil
	  return
	end
      else
	if l =~ /^=begin/
	  $ltype = "="
	  return
	end
      end
      
      until chrs.empty?
	@space_seen = FALSE
	printf "perse: %s\n", chrs.join("") if CONFIG[:DEBUG]
	@OP.match(chrs)
	printf "lex_state: %s continue: %s\n", @lex_state.id2name, @continue if CONFIG[:DEBUG]
      end
    end
    
    def identify_gvar(chrs)
      @lex_state = EXPR_END
      
      ch = chrs.shift
      case ch
      when /[_~*$?!@\/\\;,.=:<>"]/   #"
	return
	
      when "-"
	ch = chrs.shift
	return
	
      when "&", "`", "'", "+"
	return
	
      when /[1-9]/
	chrs.unshift ch
	v = "$"
	while (ch = chrs.shift) =~ /[0-9]/
	end
	chrs.unshift ch
	return
	
      when /\w/
	chrs.unshift ch
	chrs.unshift "$"
	identify_identifier(chrs)
	return
	
      else 
	chrs.unshift ch
	return
      end
    end
    
    def identify_identifier(chrs, escaped = FALSE)
      token = ""
      token.concat chrs.shift if chrs[0] =~ /[$@]/ or escaped
      while (ch = chrs.shift) =~ /\w|_/
	print ":", ch, ":" if CONFIG[:DEBUG]
	token.concat ch
      end
      chrs.unshift ch
      
      if ch == "!" or ch == "?"
	chrs.shift
	token.concat ch
      end
      # fix token
      
      if token =~ /^[$@]/ or escaped
	@lex_state = EXPR_END
	return
      end
      
      print token, "\n" if CONFIG[:DEBUG]
      if state = CLAUSE_STATE_TRANS[token]
	if @lex_state != EXPR_BEG and token =~ /^(if|unless|while|until)/
	  # modifiers
	else
	  if ENINDENT_CLAUSE.include?(token)
	    @indent += 1
	  elsif DEINDENT_CLAUSE.include?(token)
	    @indent -= 1
	  end
	end
	@lex_state = state
	return
      end
      if @lex_state == EXPR_FNAME
	@lex_state = EXPR_END
	if chrs[0] == '='
	  chrs.shift
	end
      elsif @lex_state == EXPR_BEG
	@lex_state = EXPR_ARG
      else
	@lex_state = EXPR_END
      end
    end
    
    def identify_quotation(chrs)
      ch = chrs.shift
      if lt = PARCENT_LTYPE[ch]
	ch = chrs.shift
      else
	lt = "\""
      end
      if ch !~ /\W/
	chrs.unshift ch
	next
      end
      @ltype = lt
      unless @quoted = PARCENT_PAREN[ch]
	@quoted = ch
      end
      identify_string(chrs)
    end

    def identify_number(chrs)
      @lex_state = EXPR_END
      
      ch = chrs.shift
      case ch
      when /0/
	if (ch = chrs[0]) == "x"
	  chrs.shift
	  match = /[0-9a-f_]/
	else
	  match = /[0-7_]/
	end
	while ch = chrs.shift
	  if ch !~ match
	    chrs.unshift ch
	    break
	  end
	end
	return
      end
      
      while ch = chrs.shift
	case ch
	when /[0-9]/
	when "e", "E"
	  #	type = FLOAT
	  unless (ch = chrs.shift) == "+" or ch == "-"
	    chrs.unshift ch
	  end
	when "."
	  #	type = FLOAT
	when "_"
	else
	  chrs.unshift ch
	  return
	end
      end
    end
    
    def identify_question(chrs)
      @lex_state = EXPR_END
      
      if chrs.shift == "\\" #"
	identify_escape(chrs)
      end
    end
    
    def identify_string(chrs)
      while ch = chrs.shift
	if @quoted == ch
	  if @ltype == "/"
	    if chrs[0] =~ /i|o|n|e|s/
	      chrs.shift
	    end
	  end
	  @ltype = nil
	  @quoted = nil
	  @lex_state = EXPR_END
	  break
	elsif ch == '\\' #'
	  identify_escape(chrs)
	end
      end
    end
    
    def identify_comment(chrs)
      while ch = chrs.shift
	if ch == "\\" #"
	  identify_escape(chrs)
	end
	if ch == "\n"
	  @ltype = nil
	  chrs.unshift ch
	  break
	end
      end
    end
    
    def identify_escape(chrs)
      ch = chrs.shift
      case ch
      when "\n", "\r", "\f"
	@continue = TRUE
      when "\\", "n", "t", "r", "f", "v", "a", "e", "b" #"
      when /[0-7]/
	chrs.unshift ch
	3.times do
	  ch = chrs.shift
	  case ch
	  when /[0-7]/
	  when nil
	    break
	  else
	    chrs.unshift ch
	    break
	  end
	end
      when "x"
	2.times do
	  ch = chrs.shift
	  case ch
	  when /[0-9a-fA-F]/
	  when nil
	    break
	  else
	    chrs.unshift ch
	    break
	  end
	end
      when "M"
	if (ch = chrs.shift) != '-'
	  chrs.unshift ch
	elsif (ch = chrs.shift) == "\\" #"
	  identify_escape(chrs)
	end
	return
      when "C", "c", "^"
	if ch == "C" and (ch = chrs.shift) != "-"
	  chrs.unshift ch
	elsif (ch = chrs.shift) == "\\" #"
	  identify_escape(chrs)
	end
	return
      end
    end
  end
  
  class Trie
    extend Exception2MessageMapper
    def_exception :ErrNodeNothing, "node nothing"
    def_exception :ErrNodeAlreadyExists, "node already exists"

    class Node
      # abstract node if postproc is nil.
      def initialize(preproc = nil, postproc = nil)
	@Tree = {}
	@preproc = preproc
	@postproc = postproc
      end

      attr :preproc, TRUE
      attr :postproc, TRUE
      
      def search(chrs, opt = nil)
	return self if chrs.empty?
	ch = chrs.shift
	if node = @Tree[ch]
	  node.search(chrs, opt)
	else
	  if opt
	    chrs.unshift ch
	    self.create_subnode(chrs)
	  else
	    Trie.fail ErrNodeNothing
	  end
	end
      end
      
      def create_subnode(chrs, preproc = nil, postproc = nil)
	if chrs.empty?
	  if @postproc
	    p node
	    Trie.fail ErrNodeAlreadyExists
	  else
	    print "Warn: change abstruct node to real node\n" if CONFIG[:DEBUG]
	    @preproc = preproc
	    @postproc = postproc
	  end
	  return self
	end
	
	ch = chrs.shift
	if node = @Tree[ch]
	  if chrs.empty?
	    if node.postproc
	      p node
	      Trie.fail ErrNodeAlreadyExists
	    else
	      print "Warn: change abstruct node to real node\n" if CONFIG[:DEBUG]
	      node.preproc = preproc
	      node.postproc = postproc
	    end
	  else
	    node.create_subnode(chrs, preproc, postproc)
	  end
	else
	  if chrs.empty?
	    node = Node.new(preproc, postproc)
	  else
	    node = Node.new
	    node.create_subnode(chrs, preproc, postproc)
	  end
	  @Tree[ch] = node
	end
	node
      end
      
      def match(chrs, op = "")
	print "match>: ", chrs, "op:", op, "\n" if CONFIG[:DEBUG]
	if chrs.empty?
	  if @preproc.nil? || @preproc.call(op, chrs)
	    printf "op1: %s\n", op if CONFIG[:DEBUG]
	    @postproc.call(op, chrs)
	    ""
	  else
	    nil
	  end
	else
	  ch = chrs.shift
	  if node = @Tree[ch]
	    if ret = node.match(chrs, op+ch)
	      return ch+ret
	    else
	      chrs.unshift ch
	      if @postproc and @preproc.nil? || @preproc.call(op, chrs)
		printf "op2: %s\n", op.inspect if CONFIG[:DEBUG]
		@postproc.call(op, chrs)
		return ""
	      else
		return nil
	      end
	    end
	  else
	    chrs.unshift ch
	    if @postproc and @preproc.nil? || @preproc.call(op, chrs)
	      printf "op3: %s\n", op if CONFIG[:DEBUG]
	      @postproc.call(op, chrs)
	      return ""
	    else
	      return nil
	    end
	  end
	end
      end
    end
    
    def initialize
      @head = Node.new("")
    end
    
    def def_rule(token, preproc = nil, postproc = nil)
#      print node.inspect, "\n" if CONFIG[:DEBUG]
      postproc = proc if iterator?
      node = create(token, preproc, postproc)
    end
    
    def def_rules(*tokens)
      if iterator?
	p = proc
      end
      for token in tokens
	def_rule(token, nil, p)
      end
    end
    
    def preporc(token, proc)
      node = search(token)
      node.preproc=proc
    end
    
    def postproc(token)
      node = search(token, proc)
      node.postproc=proc
    end
    
    def search(token)
      @head.search(token.split(//))
    end

    def create(token, preproc = nil, postproc = nil)
      @head.create_subnode(token.split(//), preproc, postproc)
    end
    
    def match(token)
      token = token.split(//) if token.kind_of?(String)
      ret = @head.match(token)
      printf "match end: %s:%s", ret, token.inspect if CONFIG[:DEBUG]
      ret
    end
    
    def inspect
      format("<Trie: @head = %s>", @head.inspect)
    end
  end
  
  if /^-tt(.*)$/ =~ ARGV[0]
#    Tracer.on
    case $1
    when "1"
      tr = Trie.new
      print "0: ", tr.inspect, "\n"
      tr.def_rule("=") {print "=\n"}
      print "1: ", tr.inspect, "\n"
      tr.def_rule("==") {print "==\n"}
      print "2: ", tr.inspect, "\n"
      
      print "case 1:\n"
      print tr.match("="), "\n"
      print "case 2:\n"
      print tr.match("=="), "\n"
      print "case 3:\n"
      print tr.match("=>"), "\n"
      
    when "2"
      tr = Trie.new
      print "0: ", tr.inspect, "\n"
      tr.def_rule("=") {print "=\n"}
      print "1: ", tr.inspect, "\n"
      tr.def_rule("==", proc{FALSE}) {print "==\n"}
      print "2: ", tr.inspect, "\n"
      
      print "case 1:\n"
      print tr.match("="), "\n"
      print "case 2:\n"
      print tr.match("=="), "\n"
      print "case 3:\n"
      print tr.match("=>"), "\n"
    end
    exit
  end
  
  module CONTEXT
    def _=(value)
      CONFIG[:_] = value
      eval "_=BC_APPLICATION__::CONFIG[:_]", CONFIG[:BIND]
    end
    
#    def _
#      eval "_", CONFIG[:BIND]
#    end
    
    def quit
      exit
    end
    
    def trace_load(opt = nil)
      if !opt.nil?
	CONFIG[:TRACE_LOAD] = opt
      else
	CONFIG[:TRACE_LOAD] = !CONFIG[:TRACE_LOAD]
      end
      print "Switch to load/require #{unless CONFIG[:TRACE_LOAD]; ' non';end} trace mode.\n"
      if CONFIG[:TRACE_LOAD]
	eval %{
	  class << self
	    alias load rbc_load
	    alias require rbc_require
	  end
	}
      else
	eval %{
	  class << self
	    alias load rbc_load_org
	    alias require rbc_require_org
	  end
	}
      end
      CONFIG[:TRACE_LOAD]
    end
    
    alias rbc_load_org load
    def rbc_load(file_name)
      return true if load_sub(file_name)
      raise LoadError, "No such file to load -- #{file_name}"
    end

    alias rbc_require_org require
    def rbc_require(file_name)
      rex = Regexp.new("#{Regexp.quote(file_name)}(\.o|\.rb)?")
      return false if $".find{|f| f =~ rex}

      case file_name
      when /\.rb$/
	if load_sub(file_name)
	  $".push file_name
	  return true
	end
      when /\.(so|o|sl)$/
	rbc_require_org(file_name)
      end
      
      if load_sub(f = file_name + ".rb")
	$".push f
	return true
      end
      rbc_require_org(file_name)
    end

    def load_sub(fn)
      if fn =~ /^#{Regexp.quote(File::Separator)}/
	return false unless File.exist?(fn)
	BC.new.eval_input FileInputMethod.new(fn), self, CONFIG[:BIND]
	return true
      end
      
      for path in $:
	if File.exist?(f = File.join(path, fn))
	  BC.new.eval_input FileInputMethod.new(f), self, CONFIG[:BIND]
	  return true
	end
      end
      return false
    end

    def inspect_mode(opt = nil)
      if opt
	CONFIG[:INSPECT] = opt
      else
	CONFIG[:INSPECT] = !CONFIG[:INSPECT]
      end
      print "Switch to#{unless CONFIG[:INSPECT]; ' non';end} inspect mode.\n"
      CONFIG[:INSPECT]
    end
    
    def run(bind)
      CONFIG[:BIND] = bind

      if CONFIG[:RC]
	rc = File.expand_path("~/.irbrc")
	if File.exists?(rc)
	  begin
	    load rc
	  rescue LoadError
	    print "load error: #{rc}\n"
	    print $!.type, ": ", $!, "\n"
	    for err in $@[0, $@.size - 2]
	      print "\t", err, "\n"
	    end
	  end 
	end
      end
  
      if CONFIG[:TRACE_LOAD]
	trace_load true
      end
  
      for m in CONFIG[:LOAD_MODULES]
	begin
	  require m
	rescue LoadError
	  print $@[0], ":", $!.type, ": ", $!, "\n"
	end
      end

      if !$0.equal?(CONFIG[0])
	io = FileInputMethod.new($0)
      elsif defined? Readline
	io = ReadlineInputMethod.new
      else
	io = StdioInputMethod.new
      end

      BC.new.eval_input io, self, CONFIG[:BIND]
    end
  end
  
  class InputMethod
    attr :prompt, TRUE
    
    def gets
    end
    public :gets
  end
  
  class StdioInputMethod < InputMethod
    def gets
      print @prompt
      $stdin.gets
    end
  end
  
  class FileInputMethod < InputMethod
    def initialize(file)
      @io = open(file)
    end

    def gets
      l = @io.gets
      print @prompt, l
      l
    end
  end

  if CONFIG[:USE_READLINE]
    begin
      require "readline"
      print "use readline module\n"
      class ReadlineInputMethod < InputMethod
	include Readline 
	def gets
	  if l = readline(@prompt, TRUE)
	    l + "\n"
	  else
	    l
	  end
	end
      end
    rescue LoadError
      CONFIG[:USE_READLINE] = FALSE
    end
  end
end

extend BC_APPLICATION__::CONTEXT
run(binding)

