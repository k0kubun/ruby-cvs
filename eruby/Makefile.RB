#!/usr/bin/env ruby

require "mkmf"

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG == "libruby.a"
  $LIBRUBYARG = $hdrdir + "/libruby.a"
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

prefix = #{CONFIG["prefix"]}
bindir = #{CONFIG["bindir"]}

CFLAGS = -I#$hdrdir #{CFLAGS} #{$defs.join(" ")}
LDFLAGS = #{CONFIG["LDFLAGS"]}
LIBS = #{CONFIG["LIBS"]}
INSTALL = #{CONFIG["INSTALL"]}
INSTALL_PROGRAM = #{CONFIG["INSTALL_PROGRAM"]}
INSTALL_DATA = #{CONFIG["INSTALL_DATA"]}

LIBRUBYARG = #$LIBRUBYARG
OBJS = eruby.o eruby_logo.o eruby_main.o
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
	$(CC) $(LDFLAGS) $(OBJS) $(LIBRUBYARG) $(LIBS) -o $@

eruby_logo.c: eruby_logo.gif
	./bin2c eruby_logo.gif
EOMF
ensure
  mfile.close
end
