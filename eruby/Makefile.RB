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
		     "US-ASCII"
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

$MAJOR, $MINOR, $TEENY =
  open("eruby.h").grep(/ERUBY_VERSION/)[0].scan(/(\d+).(\d+).(\d+)/)[0]

$CFLAGS = CFLAGS

$RM = "rm -f"
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
  $LIBRUBYARG.gsub!(/ruby\.mswin32\.lib/, "libruby.lib")
  $RM = "del"
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}
RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
RUBY = $(RUBY_INSTALL_NAME)

prefix = #{CONFIG["prefix"]}
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include
mandir = #{CONFIG["mandir"]}

topdir = #{$topdir}
hdrdir = #{$hdrdir}

CFLAGS = #{$CFLAGS} -I$(hdrdir)
LDFLAGS = #{CONFIG["LDFLAGS"]}
LIBS = #{CONFIG["LIBS"]}
XLDFLAGS = #{$XLDFLAGS}
DLDFLAGS = #{$DLDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
INSTALL_PROGRAM = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0755, true'
INSTALL_DATA = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0644, true'
LIBRUBYARG = #{$LIBRUBYARG}

TARGET = eruby#{CONFIG["EXEEXT"]}
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

ERUBY_OBJS = eruby.#{$OBJEXT}
OBJS = eruby_logo.#{$OBJEXT} eruby_main.#{$OBJEXT}

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(LIBERUBY_A) $(TARGET) $(MANPAGE)

install: all
	@-$(INSTALL_PROGRAM) $(TARGET) $(bindir)/$(TARGET)
	@-if [ -f $(LIBERUBY_SO) ]; then \
		$(INSTALL_DATA) $(LIBERUBY_SO) $(libdir)/$(LIBERUBY_SO); \
		cd $(libdir); \
		$(RUBY) -e 'ARGV.each{|link| File.delete link \
			if File.exist? link; \
			File.symlink "$(LIBERUBY_SO)", link}' \
			$(LIBERUBY_ALIASES); \
	fi
	@-$(INSTALL_DATA) $(LIBERUBY_A) $(libdir)/$(LIBERUBY_A)
	@-$(INSTALL_DATA) eruby.h $(includedir)/eruby.h
	@-$(INSTALL_DATA) $(MANPAGE) $(mandir)/man1/$(MANPAGE)

clean:
	#{$RM} $(TARGET)
	#{$RM} $(LIBERUBY_A)
	#{$RM} $(LIBERUBY_SO)
	#{$RM} $(LIBERUBY_ALIASES)
	#{$RM} $(ERUBY_OBJS)
	#{$RM} $(OBJS)
	#{$RM} eruby.imp
	#{$RM} *~

distclean: clean
	#{$RM} config.h Makefile

$(LIBERUBY_A): $(ERUBY_OBJS)
	#{$AR} #{$AROPT} $(ERUBY_OBJS)
	@-ranlib $@ 2> /dev/null || true

$(LIBERUBY_SO): $(ERUBY_OBJS)
	$(LDSHARED) $(DLDFLAGS) $(ERUBY_OBJS) -o $@
	@-$(RUBY) -e 'ARGV.each{|link| File.delete link if File.exist? link; \
		File.symlink "$(LIBERUBY_SO)", link}' $(LIBERUBY_ALIASES)

$(TARGET): $(OBJS) $(LIBERUBY)
	$(CC) $(LDFLAGS) $(XLDFLAGS) $(OBJS) $(LIBERUBYARG) $(LIBRUBYARG) $(LIBS) -o $@

$(MANPAGE): eruby.rd
	rd2 -r rd/rd2man-lib $< > $@
	$(RUBY) -i -pe 'gsub(/eruby\.rd/, "eruby")' $@
	$(RUBY) -i -ne 'print if ~/.+/' $@

eruby_logo.c: eruby_logo.png
	./bin2c eruby_logo.png
eruby.#{$OBJEXT}: eruby.c eruby.h config.h
eruby_logo.#{$OBJEXT}: eruby_logo.c eruby_logo.h
eruby_main.#{$OBJEXT}: eruby_main.c eruby.h eruby_logo.h
EOMF
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
