#!/usr/bin/env ruby

# Generated automatically using autoconf.rb version 0.1

require "mkmf"

$ac_help = ""
$ac_help += "  --enable-shared         build a shared library for eruby" "\n"
$ac_help += "  --with-charset=CHARSET  default charset is CHARSET [iso-8859-1]" "\n"

$ac_sed = {}
$ac_confdefs = {}
$ac_features = {}
$ac_packages = {}
  
def AC_SUBST(variable)
  $ac_sed[variable] = true
end

def AC_DEFINE(variable, value = 1)
  case value
  when true
    value = "1"
  when false
    value = "0"
  when String
    value = value.dump
  else
    value = value.inspect
  end
  $ac_confdefs[variable] = value
end

AC_GIVEN = Object.new
def AC_GIVEN.if_not_given
  # do nothing
end

AC_NOT_GIVEN = Object.new
def AC_NOT_GIVEN.if_not_given
  yield
end

def AC_ENABLE(feature, action = Proc.new)
  if $ac_features[feature]
    yield($ac_features[feature])
    return AC_GIVEN
  else
    return AC_NOT_GIVEN
  end
end

def AC_WITH(package, action = Proc.new)
  if $ac_packages[package]
    yield($ac_packages[package])
    return AC_GIVEN
  else
    return AC_NOT_GIVEN
  end
end

def AC_OUTPUT(*files)
  if $AC_LIST_HEADER
    $DEFS = "-DHAVE_CONFIG_H"
    AC_OUTPUT_HEADER($AC_LIST_HEADER)
  else
    $DEFS = $ac_confdefs.collect {|k, v| "-D#{k}=#{v}" }.join(" ")
  end
  for file in files
    print "creating ", file, "\n"
    open(File.join($srcdir, file + ".in")) do |fin|
      open(file, "w") do |fout|
	while line = fin.gets
	  line.gsub!(/@([A-Za-z_]+)@/) do |s|
	    name = $1
	    if $ac_sed.key?(name)
	      eval("$" + name) #"
	    else
	      s
	    end
	  end
	  fout.print(line)
	end
      end
    end
  end
end

def AC_OUTPUT_HEADER(header)
  print "creating ", header, "\n"
  open(File.join($srcdir, header + ".in")) do |fin|
    open(header, "w") do |fout|
      while line = fin.gets
	line.sub!(/^(#define|#undef)\s+([A-Za-z_]+).*$/) do |s|
	  name = $2
	  if $ac_confdefs.key?(name)
	    val = $ac_confdefs[name]
	    "#define #{name} #{val}"
	  else
	    s
	  end
	end
	fout.print(line)
      end
    end
  end
  $ac_confdefs.clear
end

def AC_CONFIG_HEADER(header)
  $AC_LIST_HEADER = header
end

def AC_MSG_CHECKING(feature)
  print "checking #{feature}... "
  $stdout.flush
end

def AC_MSG_RESULT(result)
  case result
  when true
    result = "yes"
  when false, nil
    result = "no"
  end
  puts(result)
end

def AC_MSG_WARN(msg)
  $stderr.print("configure.rb: warning: ", msg, "\n")
end

def AC_MSG_ERROR(msg)
  $stderr.print("configure.rb: error: ", msg, "\n")
  exit(1)
end

$stdout.sync = true

drive = File::PATH_SEPARATOR == ';' ? /\A\w:/ : /\A/
prefix = Regexp.new("\\A" + Regexp.quote(CONFIG["prefix"]))
$prefix = CONFIG["prefix"].sub(drive, '')
$exec_prefix = "$(prefix)"
$bindir = CONFIG["bindir"].sub(prefix, "$(exec_prefix)").sub(drive, '')
$libdir = CONFIG["libdir"].sub(prefix, "$(exec_prefix)").sub(drive, '')
$archdir = $archdir.sub(prefix, "$(prefix)").sub(drive, '')
$sitelibdir = $sitelibdir.sub(prefix, "$(prefix)").sub(drive, '')
$sitearchdir = $sitearchdir.sub(prefix, "$(prefix)").sub(drive, '')
$includedir = CONFIG["includedir"].sub(prefix, "$(prefix)").sub(drive, '')
$mandir = CONFIG["mandir"].sub(prefix, "$(prefix)").sub(drive, '')

$rubylibdir ||=
  $libdir + "/ruby/" + CONFIG["MAJOR"] + "." + CONFIG["MINOR"]

for option in ARGV
  if option =~ /^-.*?=(.*)/
    optarg = $1
  else
    optarg = nil
  end
  case option
  when /^--prefix/
    $prefix = optarg
  when /^--exec-prefix/
    $exec_prefix = optarg
  when /^--bindir/
    $bindir = optarg
  when /^--libdir/
    $libdir = optarg
  when /^--includedir/
    $includedir = optarg
  when /^--mandir/
    $mandir = optarg
  when /^--enable-([^=]+)/
    feature = $1
    if optarg.nil?
      optarg = "yes"
    end
    $ac_features[feature] = optarg
  when /^--with-([^=]+)/
    package = $1
    if optarg.nil?
      optarg = "yes"
    end
    $ac_packages[package] = optarg
  when "--help"
    print <<EOF
Usage: configure.rb [options]
Options: [defaults in brackets after descriptions]
Configuration:
  --help                  print this message
Directory and file names:
  --prefix=PREFIX         install architecture-independent files in PREFIX
                          [#{$prefix}]
  --exec-prefix=EPREFIX   install architecture-dependent files in EPREFIX
                          [same as prefix]
  --bindir=DIR            user executables in DIR [EPREFIX/bin]
  --libdir=DIR            object code libraries in DIR [EPREFIX/lib]
  --includedir=DIR        C header files in DIR [PREFIX/include]
  --mandir=DIR            manual pages in DIR [PREFIX/man]
EOF
    if $ac_help.length > 0
      print "--enable and --with options recognized:\n"
      print $ac_help
    end
    exit(0)
  when /^-.*/
    AC_MSG_ERROR("#{option}: invalid option; use --help to show usage")
  end
end

$srcdir = File.dirname($0)
$VPATH = ""

$RUBY_INSTALL_NAME = CONFIG["RUBY_INSTALL_NAME"]
$arch = CONFIG["arch"]
$ruby_version = Config::CONFIG["ruby_version"] ||
  CONFIG["MAJOR"] + "." + CONFIG["MINOR"]

$CC = CONFIG["CC"]
$AR = CONFIG["AR"]
$LD = "$(CC)"
$RANLIB = CONFIG["RANLIB"]

$CFLAGS = CFLAGS + " " + CONFIG["CCDLFLAGS"]
$LDFLAGS = CONFIG["LDFLAGS"]
$LIBS = CONFIG["LIBS"]
$XLDFLAGS = CONFIG["XLDFLAGS"]
$DLDFLAGS = CONFIG["DLDFLAGS"]
$LDSHARED = CONFIG["LDSHARED"]

$EXEEXT = CONFIG["EXEEXT"]
$DLEXT = CONFIG["DLEXT"]

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $RUBY_SHARED = false
  $LIBRUBYARG = "$(hdrdir)/" + $LIBRUBYARG
else
  $RUBY_SHARED = true
  $LIBRUBYARG.gsub!(/-L\./, "-L$(libdir)")
end

case PLATFORM
when /-aix/
  if $RUBY_SHARED
    $LIBRUBYARG = "-Wl,$(libdir)/" + CONFIG["LIBRUBY_SO"]
    $LIBRUBYARG.sub!(/\.so\.[.0-9]*$/, '.so')
    $XLDFLAGS = ""
  else
    $XLDFLAGS = "-Wl,-bE:$(topdir)/ruby.imp"
  end
  if $DLDFLAGS !~ /-Wl,/
    $LIBRUBYARG.gsub!(/-Wl,/, '')
    $XLDFLAGS.gsub!(/-Wl,/, '')
    $DLDFLAGS.gsub!(/-Wl,/, '')
  end
end

AC_SUBST("srcdir")
AC_SUBST("topdir")
AC_SUBST("hdrdir")
AC_SUBST("VPATH")

AC_SUBST("RUBY_INSTALL_NAME")
AC_SUBST("arch")
AC_SUBST("ruby_version")
AC_SUBST("prefix")
AC_SUBST("exec_prefix")
AC_SUBST("bindir")
AC_SUBST("libdir")
AC_SUBST("rubylibdir")
AC_SUBST("archdir")
AC_SUBST("sitedir")
AC_SUBST("sitelibdir")
AC_SUBST("sitearchdir")
AC_SUBST("includedir")
AC_SUBST("mandir")

AC_SUBST("CC")
AC_SUBST("AR")
AC_SUBST("LD")
AC_SUBST("RANLIB")

AC_SUBST("CFLAGS")
AC_SUBST("DEFS")
AC_SUBST("LDFLAGS")
AC_SUBST("LIBS")
AC_SUBST("XLDFLAGS")
AC_SUBST("DLDFLAGS")
AC_SUBST("LDSHARED")

AC_SUBST("OBJEXT")
AC_SUBST("EXEEXT")
AC_SUBST("DLEXT")

AC_SUBST("LIBRUBYARG")

AC_MSG_CHECKING("Ruby version")
AC_MSG_RESULT(RUBY_VERSION)
if RUBY_VERSION < "1.6"
  AC_MSG_ERROR("eruby requires Ruby 1.6.x or later.")
end

$MAJOR, $MINOR, $TEENY =
  open(File.join($srcdir, "eruby.h")).grep(/ERUBY_VERSION/)[0].scan(/(\d+).(\d+).(\d+)/)[0]
$MAJOR_MINOR = format("%02d", ($MAJOR.to_i * 10 + $MINOR.to_i))
AC_SUBST("MAJOR")
AC_SUBST("MINOR")
AC_SUBST("TEENY")
AC_SUBST("MAJOR_MINOR")

AC_MSG_CHECKING("for default charset")
$DEFAULT_CHARSET = "iso-8859-1"
AC_WITH("charset") { |withval|
  $DEFAULT_CHARSET = withval
}
AC_MSG_RESULT($DEFAULT_CHARSET)
AC_DEFINE("ERUBY_DEFAULT_CHARSET", $DEFAULT_CHARSET)

AC_MSG_CHECKING("whether enable shared")
$ENABLE_SHARED = false
AC_ENABLE("shared") { |enableval|
  if enableval == "yes"
    if PLATFORM =~ /-mswin32/
      AC_MSG_ERROR("can't enable shared on mswin32")
    end
    $ENABLE_SHARED = true
  end
}
AC_MSG_RESULT($ENABLE_SHARED)

$LIBERUBY_A = "liberuby.a"
$LIBERUBY = "${LIBERUBY_A}"
$LIBERUBYARG="$(LIBERUBY_A)"

$LIBERUBY_SO = "liberuby.#{CONFIG['DLEXT']}.$(MAJOR).$(MINOR).$(TEENY)"
$LIBERUBY_ALIASES = "liberuby.#{CONFIG['DLEXT']}"

if $ENABLE_SHARED
  $LIBERUBY = "${LIBERUBY_SO}"
  $LIBERUBYARG = "-L. -leruby"
  case PLATFORM
  when /-sunos4/
    $LIBERUBY_ALIASES = "liberuby.so.$(MAJOR).$(MINOR) liberuby.so"
  when /-linux/
    $DLDFLAGS = '-Wl,-soname,liberuby.so.$(MAJOR).$(MINOR)'
    $LIBERUBY_ALIASES = "liberuby.so.$(MAJOR).$(MINOR) liberuby.so"
  when /-(freebsd|netbsd)/
    $LIBERUBY_SO = "liberuby.so.$(MAJOR).$(MINOR)"
    if PLATFORM =~ /elf/ || PLATFORM =~ /-freebsd[3-9]/
      $LIBERUBY_SO = "liberuby.so.$(MAJOR_MINOR)"
      $LIBERUBY_ALIASES = "liberuby.so"
    else
      $LIBERUBY_ALIASES = "liberuby.so.$(MAJOR) liberuby.so"
    end
  when /-solaris/
    $XLDFLAGS = "-R$(prefix)/lib"
  when /-hpux/
    $XLDFLAGS = "-Wl,+s,+b,$(prefix)/lib"
    $LIBERUBY_SO = "liberuby.sl.$(MAJOR).$(MINOR).$(TEENY)"
    $LIBERUBY_ALIASES = 'liberuby.sl.$(MAJOR).$(MINOR) liberuby.sl'
  when /-aix/
    $DLDFLAGS = '-Wl,-bE:eruby.imp'
    $LIBERUBYARG = "-L$(prefix)/lib -Wl,liberuby.so"
    if $DLDFLAGS !~ /-Wl,/
      $LIBRUBYARG.gsub!(/-Wl,/, '')
      $XLDFLAGS.gsub!(/-Wl,/, '')
      $DLDFLAGS.gsub!(/-Wl,/, '')
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
  $AR = "lib"
  $AROPT = "/out:$@"
  $LIBERUBY_A = "liberuby.lib"
  $LIBERUBY = "$(LIBERUBY_A)"
  $LIBRUBYARG.gsub!(CONFIG["RUBY_SO_NAME"] + ".lib", CONFIG["LIBRUBY_A"])
  if /nmake/i =~ $make
    $LD = "(set LIB=$(libdir:/=\\);$(LIB))& $(CC)"
    $VPATH = "{$(VPATH)}"
  else
    $LD = "env LIB='$(subst /,\\\\,$(libdir));$(LIB)' $(CC)"
  end
else
  $AROPT = "rcu $@"
end

AC_SUBST("LIBERUBY_A")
AC_SUBST("LIBERUBY")
AC_SUBST("LIBERUBYARG")
AC_SUBST("LIBERUBY_SO")
AC_SUBST("LIBERUBY_ALIASES")
AC_SUBST("AROPT")

if $DLEXT != $OBJEXT
  $MKDLLIB = ""
  if /mswin32/ =~ RUBY_PLATFORM
    if /nmake/i =~ $make
      $MKDLLIB << "set LIB=$(LIBPATH:/=\\);$(LIB)\n\t"
    else
      $MKDLLIB << "\tenv LIB='$(subst /,\\\\,$(LIBPATH));$(LIB)' \\\n\t"
    end
  end
  $MKDLLIB << "$(LDSHARED) $(DLDFLAGS) -o $(DLLIB) $(EXT_OBJS) $(LIBERUBYARG) $(LIBS)"
else
  case RUBY_PLATFORM
  when "m68k-human"
    $MKDLLIB = "ar cru $(DLLIB) $(EXT_OBJS) $(LIBS)"
  else
    $MKDLLIB = "ld $(DLDFLAGS) -r -o $(DLLIB) $(EXT_OBJS) $(LIBERUBYARG) $(LIBS)"
  end
end

AC_SUBST("MKDLLIB")

AC_CONFIG_HEADER("config.h")
AC_OUTPUT("Makefile")
