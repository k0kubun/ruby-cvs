#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

$prefix = nil
$shared = false
$LIBERUBY = "liberuby.a"
$LIBERUBYARG = "liberuby.a"

opts = GetoptLong.new(["--prefix", GetoptLong::REQUIRED_ARGUMENT],
		      ["--enable-shared", GetoptLong::NO_ARGUMENT],
		      ["--help", GetoptLong::NO_ARGUMENT],)
begin
  for name, arg in opts
    case name
    when "--prefix"
      $prefix = arg
    when "--enable-shared"
      $shared = true
      $LIBERUBY = "liberuby.so"
      $LIBERUBYARG = "-L. -leruby"
    when "--help"
      $stderr.print <<EOF
Usage: #{$0} [options]
Configuration:
  --help		print this message

  --prefix=PREFIX	install files in PREFIX
  --enable-shared	build a shared library for eRuby
EOF
      exit 1
    end
  end
rescue
  exit 1
end

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

prefix = #{$prefix || CONFIG["prefix"]}
bindir = $(prefix)/bin
libdir = $(prefix)/lib

hdrdir = #{$hdrdir}

CFLAGS = -I$(hdrdir) #{CFLAGS}
LDFLAGS = #{CONFIG["LDFLAGS"]}
LIBS = #{CONFIG["LIBS"]}
DLDFLAGS = #{$DLDFLAGS} #{$LDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
INSTALL = #{CONFIG["INSTALL"]}
INSTALL_PROGRAM = #{CONFIG["INSTALL_PROGRAM"]}
INSTALL_DATA = #{CONFIG["INSTALL_DATA"]}

MAINOBJ = eruby.o
OBJS = eruby_logo.o eruby_main.o
RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
LIBRUBYARG = #$LIBRUBYARG
LIBERUBY = #{$LIBERUBY}
LIBERUBYARG = #{$LIBERUBYARG}
TARGET = eruby

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET) $(LIBERUBY)

install: all
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)/$(TARGET)
	if [ -f liberuby.so ]; then $(INSTALL_PROGRAM) liberuby.so $(libdir); fi

clean:
	rm -f $(TARGET) $(LIBERUBY) $(OBJS) *~

distclean: clean
	rm -f Makefile

liberuby.a: $(MAINOBJ)
	ar rcu $@ $<

liberuby.so: $(MAINOBJ)
	$(LDSHARED) -o $@ $<

$(TARGET): $(OBJS) $(LIBERUBY)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBERUBYARG) $(LIBRUBYARG) $(LIBS) -o $@

eruby_logo.c: eruby_logo.gif
	./bin2c eruby_logo.gif
EOMF
ensure
  mfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
