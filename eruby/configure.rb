#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

def usage
  $stderr.printf <<EOS, $0
usage: %s [options]

  --help                        print this message
  --default-charset=CHARSET     default charset value
  --enable-shared               build a shared library for Ruby.
EOS
end

def create_file(filename, config)
  print "creating ", filename, "\n"
  open(filename + ".in") do |min|
    open(filename, "w") do |mout|
      while line = min.gets
	line.gsub!(/@.*?@/) do |s|
	  key = s[1..-2]
	  if config.key?(key)
	    config[key]
	  else
	    s
	  end
	end
	mout.print(line)
      end
    end
  end
end

$DEFAULT_CHARSET = case $KCODE
		   when /^e/i
		     "EUC-JP"
		   when /^s/i
		     "SHIFT_JIS"
		   when /^u/i
		     "UTF-8"
		   else
		     "ISO-8859-1"
		   end
$ENABLE_SHARED = false

parser = GetoptLong.new
parser.set_options(["--help", GetoptLong::NO_ARGUMENT],
		   ["--default-charset", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--enable-shared", GetoptLong::OPTIONAL_ARGUMENT])

begin
  parser.each_option do |name, arg|
    case name
    when "--default-charset"
      $DEFAULT_CHARSET = arg
    when "--enable-shared"
      $ENABLE_SHARED = true
    when "--help"
      usage
      exit(1)
    end
  end
rescue
  usage
  exit(1)
end

config = {}

config["srcdir"] = File.dirname($0)
config["VPATH"] = ""

config["CC"] = CONFIG["CC"]
config["AR"] = CONFIG["AR"]
config["AROPT"] = "rcu $@"
config["LD"] = "$(CC)"
config["RANLIB"] = CONFIG["RANLIB"]

config["RUBY_INSTALL_NAME"] = CONFIG["RUBY_INSTALL_NAME"]

config["prefix"] = CONFIG["prefix"]

prefix = Regexp.new("\\A" + Regexp.quote(config["prefix"]))
config["bindir"] = CONFIG["bindir"].sub(prefix, "$(prefix)")
config["libdir"] = CONFIG["libdir"].sub(prefix, "$(prefix)")
config["sitelibdir"] = $sitelibdir
config["includedir"] = CONFIG["includedir"].sub(prefix, "$(prefix)")
config["mandir"] = CONFIG["mandir"].sub(prefix, "$(prefix)")

drive = File::PATH_SEPARATOR == ';' ? /\A\w:/ : /\A/
config["archdir"] = $archdir.sub(drive, '')
config["sitearchdir"] = $sitearchdir.sub(drive, '')

config["topdir"] = $topdir
config["hdrdir"] = $hdrdir

config["CFLAGS"] = CFLAGS + " " + CONFIG["CCDLFLAGS"]
config["LDFLAGS"] = CONFIG["LDFLAGS"]
config["LIBS"] = CONFIG["LIBS"]
config["XLDFLAGS"] = $XLDFLAGS
config["DLDFLAGS"] = $DLDFLAGS
config["LDSHARED"] = CONFIG["LDSHARED"]

begin
  config["LIBRUBYARG"] = Config.expand(CONFIG["LIBRUBYARG"])
rescue
  config["LIBRUBYARG"] = CONFIG["LIBRUBYARG"]
end
if config["LIBRUBYARG"] =~ /\.a$/
  $RUBY_SHARED = false
  config["LIBRUBYARG"] = $hdrdir + "/" + config["LIBRUBYARG"]
else
  $RUBY_SHARED = true
  config["LIBRUBYARG"].gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

config["LIBERUBY_A"] = "liberuby.a"
config["LIBERUBY"] = "${LIBERUBY_A}"
config["LIBERUBYARG"]="$(LIBERUBY_A)"

config["LIBERUBY_SO"] = "liberuby.#{CONFIG['DLEXT']}.$(MAJOR).$(MINOR).$(TEENY)"
config["LIBERUBY_ALIASES"] = "liberuby.#{CONFIG['DLEXT']}"

if $ENABLE_SHARED
  config["LIBERUBY"] = "${LIBERUBY_SO}"
  config["LIBERUBYARG"] = "-L. -leruby"
  case PLATFORM
  when /-sunos4/
    config["LIBERUBY_ALIASES"] = "liberuby.so.$(MAJOR).$(MINOR) liberuby.so"
  when /-linux/
    config["DLDFLAGS"] = '-Wl,-soname,liberuby.so.$(MAJOR).$(MINOR)'
    config["LIBERUBY_ALIASES"] = "liberuby.so.$(MAJOR).$(MINOR) liberuby.so"
  when /-(freebsd|netbsd)/
    config["LIBERUBY_SO"] = "liberuby.so.$(MAJOR).$(MINOR)"
    if PLATFORM =~ /elf/ || PLATFORM =~ /-freebsd[3-9]/
      config["LIBERUBY_SO"] = "liberuby.so.$(MAJOR_MINOR)"
      config["LIBERUBY_ALIASES"] = "liberuby.so"
    else
      config["LIBERUBY_ALIASES"] = "liberuby.so.$(MAJOR) liberuby.so"
    end
  when /-solaris/
    config["XLDFLAGS"] = "-R$(prefix)/lib"
  when /-hpux/
    config["XLDFLAGS"] = "-Wl,+s,+b,$(prefix)/lib"
    config["LIBERUBY_SO"] = "liberuby.sl.$(MAJOR).$(MINOR).$(TEENY)"
    config["LIBERUBY_ALIASES"] = 'liberuby.sl.$(MAJOR).$(MINOR) liberuby.sl'
  when /-aix/
    config["DLDFLAGS"] = '-Wl,-bE:eruby.imp'
    if $RUBY_SHARED
      config["LIBRUBYARG"] = "-Wl," + CONFIG["libdir"] + "/" + CONFIG["LIBRUBY_SO"]
      config["LIBRUBYARG"].sub!(/\.so\.[.0-9]*$/, '.so')
      config["XLDFLAGS"] = ""
    else
      config["XLDFLAGS"] = "-Wl,-bE:#{$topdir}/ruby.imp"
    end
    config["LIBERUBYARG"] = "-L$(prefix)/lib -Wl,liberuby.so"
    if CONFIG["DLDFLAGS"] !~ /-Wl,/
      config["LIBRUBYARG"].gsub!(/-Wl,/, '')
      config["XLDFLAGS"].gsub!(/-Wl,/, '')
      config["DLDFLAGS"].gsub!(/-Wl,/, '')
    end
    print "creating eruby.imp\n"
    ifile = open("eruby.imp", "w")
    begin
      ifile.write <<EOIF
#!
ruby_filename
eruby_mode
eruby_noheader
eruby_charset
EOIF
    ensure
      ifile.close
    end
  end
end

if PLATFORM =~ /-mswin32/
  config["AR"] = "lib"
  config["AROPT"] = "/out:$@"
  config["LIBERUBY_A"] = "liberuby.lib"
  config["LIBERUBY"] = "$(LIBERUBY_A)"
  config["LIBRUBYARG"].gsub!(CONFIG["RUBY_SO_NAME"] + ".lib", CONFIG["LIBRUBY_A"])
  if /nmake/i =~ $make
    config["LD"] = "(set LIB=$(libdir:/=\\);$(LIB))& $(CC)"
    config["VPATH"] = "{$(VPATH)}"
  else
    config["LD"] = "env LIB='$(subst /,\\\\,$(libdir));$(LIB)' $(CC)"
  end
end

config["MAJOR"], config["MINOR"], config["TEENY"] =
  open(File.join(config["srcdir"], "eruby.h")).grep(/ERUBY_VERSION/)[0].scan(/(\d+).(\d+).(\d+)/)[0]
config["MAJOR_MINOR"] = (config["MAJOR"].to_i * 10 + config["MINOR"].to_i).to_s

config["OBJEXT"] = $OBJEXT
config["EXEEXT"] = CONFIG["EXEEXT"]
config["DLEXT"] = CONFIG["DLEXT"]

config["DEFAULT_CHARSET"] = $DEFAULT_CHARSET

open("Makefile.in") do |min|
  open("Makefile", "w") do |mout|
    while line = min.gets
      line.gsub!(/@.*?@/) do |s|
	key = s[1..-2]
	if config.key?(key)
	  config[key]
	else
	  s
	end
      end
      mout.print(line)
    end
  end
end

if CONFIG["DLEXT"] != $OBJEXT
  config["MKDLLIB"] = ""
  if /mswin32/ =~ RUBY_PLATFORM
    if /nmake/i =~ $make
      config["MKDLLIB"] << "set LIB=$(LIBPATH:/=\\);$(LIB)\n\t"
    else
      config["MKDLLIB"] << "\tenv LIB='$(subst /,\\\\,$(LIBPATH));$(LIB)' \\\n\t"
    end
  end
  config["MKDLLIB"] << "$(LDSHARED) $(DLDFLAGS) -o $(DLLIB) $(EXT_OBJS) $(LIBERUBYARG) $(LIBS)"
else
  case RUBY_PLATFORM
  when "m68k-human"
    config["MKDLLIB"] = "ar cru $(DLLIB) $(EXT_OBJS) $(LIBS)"
  else
    config["MKDLLIB"] = "ld $(DLDFLAGS) -r -o $(DLLIB) $(EXT_OBJS) $(LIBERUBYARG) $(LIBS)"
  end
end

create_file("config.h", config)
create_file("Makefile", config)

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
