#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

ENV["PATH"] += ":/sbin:/usr/sbin:/usr/local/sbin:\
/usr/apache/bin:/usr/local/apache/bin"

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
end

$APACHE_CFLAGS = `apxs -q CFLAGS`
if $APACHE_CFLAGS.size > 0
  $CFLAGS = ($APACHE_CFLAGS.split(/[ \t]+/) | CFLAGS.split(/[ \t]+/)).join(" ")
else
  $CFLAGS = CFLAGS
end
$APACHE_INCLUDEDIR = `apxs -q INCLUDEDIR`
$APACHE_LIBEXECDIR = `apxs -q LIBEXECDIR`

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

CFLAGS = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} -I#{$APACHE_INCLUDEDIR} \\
	 #{$CFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
DLDFLAGS = #{CONFIG["DLDFLAGS"]}
INSTALL = #{CONFIG["INSTALL"]}
INSTALL_DATA = #{CONFIG["INSTALL_DATA"]}

RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
LIBRUBYARG = #$LIBRUBYARG
OBJS = mod_ruby.o ruby_config.o apachelib.o eruby.o
TARGET = mod_ruby.so

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

install: $(TARGET)
	$(INSTALL_DATA) $(TARGET) #{$APACHE_LIBEXECDIR}

clean:
	rm -f $(TARGET) $(OBJS) *~

distclean: clean
	rm -f Makefile

$(TARGET): $(OBJS)
	$(LDSHARED) $(DLDFLAGS) -o $(TARGET) $(OBJS) $(LIBRUBYARG)

mod_ruby.o: mod_ruby.c mod_ruby.h ruby_config.h apachelib.h eruby.h
ruby_config.o: ruby_config.c mod_ruby.h ruby_config.h
apachelib.o: apachelib.c apachelib.h
eruby.o: eruby.c eruby.h
EOMF
ensure
  mfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
