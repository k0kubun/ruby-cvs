# !/usr/bin/ruby
# $Id$
# Copyright (C) 1998-1999  Network Applied Communication Laboratory, Inc.

require "mkmf"
require "getopts"

getopts(nil, "apachehdr:", "apachelibexec:")

$apachehdr = $OPT_apachehdr ||
  Dir["/usr/local/apache/include"][0] ||
  Dir["/usr/apache/include"][0] ||
  Dir["/usr/include/apache-1.3"][0]
$apachelibexec = $OPT_apachelibexec || 
  Dir["/usr/local/apache/libexec"][0] ||
  Dir["/usr/apache/libexec"][0] ||
  Dir["/usr/lib/apache/1.3"][0]

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG == "libruby.a"
  $LIBRUBYARG = $hdrdir + "/libruby.a"
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
apachehdr = #{$apachehdr}
apachelibexec = #{$apachelibexec}
rubyhdr = #{$hdrdir}

CC = #{CONFIG["CC"]}
CFLAGS = #{CONFIG["CCDLFLAGS"]} -I$(rubyhdr) -I$(apachehdr) #{CFLAGS} #{$defs.join(" ")}
DLDFLAGS = #{CONFIG["DLDFLAGS"]}
LDSHARED = #{CONFIG["LDSHARED"]}
INSTALL = #{CONFIG["INSTALL"]}
INSTALL_DATA = #{CONFIG["INSTALL_DATA"]}

LIBRUBYARG = #$LIBRUBYARG
OBJS = ruby_module.o ruby_config.o apachelib.o
TARGET = mod_ruby.#{CONFIG["DLEXT"]}

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

install: $(TARGET)
	$(INSTALL_DATA) $(TARGET) $(apachelibexec)

clean:
	rm -f $(TARGET) $(OBJS) *~

distclean: clean
	rm -f Makefile

$(TARGET): $(OBJS)
	$(LDSHARED) $(DLDFLAGS) -o $(TARGET) $(OBJS) $(LIBRUBYARG)

ruby_module.o: ruby_module.c ruby_module.h ruby_config.h apachelib.h
ruby_config.o: ruby_config.c ruby_module.h ruby_config.h
apachelib.o: apachelib.c apachelib.h
EOMF
ensure
  mfile.close
end

unless $apachehdr && $apachelibexec
  print <<EOF
I could not find your apache directory.
Please edit Makefile manually.
EOF
end
