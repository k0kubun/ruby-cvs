#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
else
  $LIBRUBYARG.gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

prefix = #{CONFIG["prefix"]}
bindir = $(prefix)/bin
libdir = $(prefix)/lib

hdrdir = #{$hdrdir}

CFLAGS = -I$(hdrdir) #{CFLAGS}
LDFLAGS = #{CONFIG["LDFLAGS"]}
LIBS = #{CONFIG["LIBS"]}
XLDFLAGS = #{CONFIG["XLDFLAGS"]}
DLDFLAGS = #{$DLDFLAGS} #{$LDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
INSTALL_PROGRAM = ruby -r ftools -e 'File.install ARGV[0], ARGV[1], 0755, true'

OBJS = eruby.o eruby_logo.o eruby_main.o
RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
LIBRUBYARG = #$LIBRUBYARG
TARGET = eruby

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

install: all
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) *~

distclean: clean
	rm -f Makefile

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(XLDFLAGS) $(OBJS) $(LIBRUBYARG) $(LIBS) -o $@

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
