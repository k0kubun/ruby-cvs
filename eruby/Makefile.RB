#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

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

def usage
  $stderr.printf <<EOS, $0
usage: %s [options]

  --help                        print this message
  --default-charset=CHARSET     default charset value
  --enable-shared               build a shared library for Ruby.
EOS
end

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

srcdir = File.dirname($0)
$MAJOR, $MINOR, $TEENY =
  open(File.join(srcdir, "eruby.h")).grep(/ERUBY_VERSION/)[0].scan(/(\d+).(\d+).(\d+)/)[0]

$CFLAGS = CFLAGS

$AR = "ar"
$AROPT = "rcu $@"

begin
  $LIBRUBYARG = Config.expand(CONFIG["LIBRUBYARG"])
rescue
  $LIBRUBYARG = CONFIG["LIBRUBYARG"]
end
if $LIBRUBYARG =~ /\.a$/
  $RUBY_SHARED = false
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
else
  $RUBY_SHARED = true
  $LIBRUBYARG.gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

$DLDFLAGS = CONFIG["DLDFLAGS"]
$XLDFLAGS = CONFIG["XLDFLAGS"]

$LIBERUBY_A = "liberuby.a"
$LIBERUBY = "${LIBERUBY_A}"
$LIBERUBYARG="$(LIBERUBY_A)"

$LIBERUBY_SO = "liberuby.#{CONFIG['DLEXT']}.$(MAJOR).$(MINOR).$(TEENY)"
$LIBERUBY_ALIASES = "liberuby.#{CONFIG['DLEXT']}"
if $ENABLE_SHARED
  if $CFLAGS.index(CONFIG["CCDLFLAGS"]).nil?
    $CFLAGS = $CFLAGS + " " + CONFIG["CCDLFLAGS"]
  end
  $LIBERUBY = "${LIBERUBY_SO}"
  $LIBERUBYARG="-L. -leruby"
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
    $LIBRUBY_SO = "liberuby.sl.$(MAJOR).$(MINOR).$(TEENY)"
    $LIBRUBY_ALIASES = 'liberuby.sl.$(MAJOR).$(MINOR) liberuby.sl'
  when /-aix/
    $DLDFLAGS='-Wl,-bE:eruby.imp'
    if $RUBY_SHARED
      $LIBRUBYARG = "-Wl," + CONFIG["libdir"] + "/" + CONFIG["LIBRUBY_SO"]
      $LIBRUBYARG.sub!(/\.so\.[.0-9]*$/, '.so')
      $XLDFLAGS = ""
    else
      $XLDFLAGS = "-Wl,-bE:#{$topdir}/ruby.imp"
    end
    $LIBERUBYARG = "-L$(prefix)/lib -Wl,liberuby.so"
    if CONFIG["DLDFLAGS"] !~ /-Wl,/
      $LIBRUBYARG.gsub!(/-Wl,/, '')
      $XLDFLAGS.gsub!(/-Wl,/, '')
      $DLDFLAGS.gsub!(/-Wl,/, '')
    end
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
    LD = "(set LIB=$(libdir:/=\\);$(LIB))& $(CC)"
    $VPATH = "{$(VPATH)}"
  else
    LD = "env LIB='$(subst /,\\\\,$(libdir));$(LIB)' $(CC)"
  end
else
  LD = "$(CC)"
end

prefix = CONFIG["prefix"]
Prefixed = Regexp.new(Regexp.quote(prefix))
def prefixed(name)
  CONFIG[name].sub(Prefixed, "$(prefix)")
end

mfile = open("Makefile", "w")
begin
  drive = File::PATH_SEPARATOR == ';' ? /\A\w:/ : /\A/
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

srcdir = #{srcdir}
VPATH = $(srcdir)
CC = #{CONFIG["CC"]}
AR = #{CONFIG["AR"]}
LD = #{LD}
RANLIB = #{CONFIG["RANLIB"]}
RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
RUBY = $(RUBY_INSTALL_NAME)

DESTDIR = 

prefix = $(DESTDIR)#{prefix}
bindir = $(DESTDIR)#{prefixed("bindir")}
libdir = $(DESTDIR)#{prefixed("libdir")}
includedir = $(DESTDIR)#{prefixed("includedir")}
archdir = $(DESTDIR)#{$archdir.sub(drive, '')}
sitearchdir = $(DESTDIR)#{$sitearchdir.sub(drive, '')}
mandir = $(DESTDIR)#{prefixed("mandir")}

topdir = #{$topdir}
hdrdir = #{$hdrdir}

CFLAGS = #{CONFIG["CCDLFLAGS"]} #{$CFLAGS} -I. -I$(hdrdir)
LDFLAGS = #{CONFIG["LDFLAGS"]}
LIBS = $(LIBERUBYARG) $(LIBRUBYARG) #{CONFIG["LIBS"]}
XLDFLAGS = #{$XLDFLAGS}
DLDFLAGS = #{$DLDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
INSTALL_PROGRAM = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0755, true'
INSTALL_DLLIB = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0555, true'
INSTALL_DATA = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0644, true'
INSTALL_DIR = $(RUBY) -r ftools -e 'File.makedirs(*ARGV)'
LIBRUBYARG = #{$LIBRUBYARG}

RM = ruby -e "ARGV.each {|f|File.unlink(f) rescue Errno}"

ERUBY = eruby#{CONFIG["EXEEXT"]}
DLLIB = eruby.#{CONFIG["DLEXT"]}
MANPAGE = eruby.1

MAJOR = #{$MAJOR}
MINOR = #{$MINOR}
TEENY = #{$TEENY}
MAJOR_MINOR = #{$MAJOR.to_i * 10 + $MINOR.to_i}

LIBERUBY_A = #{$LIBERUBY_A}
LIBERUBY_SO = #{$LIBERUBY_SO}
LIBERUBY_ALIASES = #{$LIBERUBY_ALIASES}
LIBERUBY = #{$LIBERUBY}
LIBERUBYARG = #{$LIBERUBYARG}
.SUFFIXES: .#{$OBJEXT} .c .rd

LIBERUBY_OBJS = eruby_lib.#{$OBJEXT}
OBJS = eruby_logo.#{$OBJEXT} eruby_main.#{$OBJEXT}
EXT_OBJS = eruby_ext.#{$OBJEXT}

#{$VPATH}.c.#{$OBJEXT}:
	$(CC) $(CFLAGS) -c $<

all: $(LIBERUBY_A) $(ERUBY) $(DLLIB) $(MANPAGE)

install: all
	@-$(INSTALL_DIR) $(bindir)
	@-$(INSTALL_DIR) $(libdir)
	@-$(INSTALL_DIR) $(includedir)
	@-$(INSTALL_DIR) $(mandir)
	@-$(INSTALL_DIR) $(archdir)
	@-$(INSTALL_PROGRAM) $(ERUBY) $(bindir)/$(ERUBY)
	@-if [ -f $(LIBERUBY_SO) ]; then \
		$(INSTALL_DATA) $(LIBERUBY_SO) $(libdir)/$(LIBERUBY_SO); \
		cd $(libdir); \
		$(RUBY) -e 'ARGV.each{|link| File.delete link \
			if File.exist? link; \
			File.symlink "$(LIBERUBY_SO)", link}' \
			$(LIBERUBY_ALIASES); \
	fi
	@-$(INSTALL_DATA) $(LIBERUBY_A) $(libdir)/$(LIBERUBY_A)
	@-$(INSTALL_DATA) #{srcdir}/eruby.h $(includedir)/eruby.h
	@-$(INSTALL_DATA) #{srcdir}/$(MANPAGE) $(mandir)/man1/$(MANPAGE)
	@-$(INSTALL_DLLIB) $(DLLIB) $(archdir)

clean:
	$(RM) $(ERUBY)
	$(RM) $(DLLIB)
	$(RM) $(LIBERUBY_A)
	$(RM) $(LIBERUBY_SO)
	$(RM) $(LIBERUBY_ALIASES)
	$(RM) $(LIBERUBY_OBJS)
	$(RM) $(OBJS)
	$(RM) $(EXT_OBJS)
	$(RM) eruby.imp

distclean: clean
	$(RM) config.h
	$(RM) Makefile
	$(RM) *~

$(LIBERUBY_A): $(LIBERUBY_OBJS)
	$(AR) #{$AROPT} $(LIBERUBY_OBJS)
	@-$(RANLIB) $@ 2> /dev/null || true

$(LIBERUBY_SO): $(LIBERUBY_OBJS)
	$(LDSHARED) $(DLDFLAGS) $(LIBERUBY_OBJS) -o $@
	@-$(RUBY) -e 'ARGV.each{|link| File.delete link if File.exist? link; \
		File.symlink "$(LIBERUBY_SO)", link}' $(LIBERUBY_ALIASES)

$(ERUBY): $(OBJS) $(LIBERUBY)
	$(LD) $(LDFLAGS) $(XLDFLAGS) $(OBJS) $(LIBS) -o $@

$(MANPAGE): #{$VPATH}eruby.rd

#{$VPATH}.rd#{$VPATH}.1:
	$(RUBY) -S rd2 -r rd/rd2man-lib -o $* $<
	$(RUBY) -i~ -ne 'print if ~/.+/' $@
	$(RM) $@~

eruby_logo.c: #{$VPATH}eruby_logo.png
	$(srcdir)/bin2c $(srcdir)/eruby_logo.png
eruby_lib.#{$OBJEXT}: #{$VPATH}eruby_lib.c #{$VPATH}eruby.h config.h
eruby_logo.#{$OBJEXT}: #{$VPATH}eruby_logo.c #{$VPATH}eruby_logo.h
eruby_main.#{$OBJEXT}: #{$VPATH}eruby_main.c #{$VPATH}eruby.h #{$VPATH}eruby_logo.h
eruby_ext.#{$OBJEXT}: #{$VPATH}eruby_ext.c #{$VPATH}eruby.h

EOMF
  if CONFIG["DLEXT"] != $OBJEXT
    mfile.print "$(DLLIB): $(EXT_OBJS) $(LIBERUBY)\n"
    if /mswin32/ =~ RUBY_PLATFORM
      if /nmake/i =~ $make
	mfile.print "\tset LIB=$(LIBPATH:/=\\);$(LIB)\n"
      else
	mfile.print "\tenv LIB='$(subst /,\\\\,$(LIBPATH));$(LIB)' \\\n"
      end
    end
    mfile.print "\t$(LDSHARED) $(DLDFLAGS) -o $(DLLIB) $(EXT_OBJS) $(LIBS)\n"
  else
    mfile.print "$(DLLIB): $(EXT_OBJS) $(LIBERUBY)\n"
    case RUBY_PLATFORM
    when "m68k-human"
      mfile.printf "ar cru $(DLLIB) $(EXT_OBJS) $(LIBS)\n"
    else
      mfile.printf "ld $(DLDFLAGS) -r -o $(DLLIB) $(EXT_OBJS) $(LIBS)\n"
    end
  end

ensure
  mfile.close
end

cfile = open("config.h", "w")
begin
  cfile.write <<EOCF
/* Generated automatically by Makefile.RB. */

#define ERUBY_DEFAULT_CHARSET "#{$DEFAULT_CHARSET}"
EOCF
ensure
  cfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
