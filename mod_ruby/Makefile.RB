#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

ENV["PATH"] += ":/sbin:/usr/sbin:/usr/local/sbin:\
/usr/apache/bin:/usr/local/apache/bin"

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $shared = false
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
else
  $shared = true
  $LIBRUBYARG.gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

if PLATFORM =~ /-aix/
  $DLDFLAGS = "-Wl,-bE:mod_ruby.imp -Wl,-bI:httpd.exp -Wl,-bM:SRE -Wl,-bnoentry -lc"
  if $shared
    $LIBRUBYARG = CONFIG["libdir"] + "/" + CONFIG["LIBRUBY_SO"]
    $LIBRUBYARG.sub!(/\.so\.[.0-9]*$/, '.so')
  else
    $DLDFLAGS = "-Wl,-bE:$(topdir)/ruby.imp " + $DLDFLAGS
  end
  if CONFIG["DLDFLAGS"] !~ /-Wl,/
    $DLDFLAGS.gsub!(/-Wl,/, '')
  end
  ifile = open("mod_ruby.imp", "w")
  begin
    ifile.write <<EOIF
#!
ruby_module
EOIF
  ensure
    ifile.close
  end
  print <<EOM
To build mod_ruby on the AIX platform, you need to have the apache
export file `httpd.exp' in the current directory.
Please copy <apache-src-directory>/support/httpd.exp to the current
directory before making `mod_ruby.so'.
EOM
else
  $DLDFLAGS = CONFIG["DLDFLAGS"]
end

$APACHE_CFLAGS = `apxs -q CFLAGS`
if $APACHE_CFLAGS.size > 0
  $CFLAGS = ($APACHE_CFLAGS.split(/[ \t]+/) | CFLAGS.split(/[ \t]+/)).join(" ")
else
  $CFLAGS = CFLAGS
end
$APACHE_INCLUDEDIR = `apxs -q INCLUDEDIR`
$APACHE_LIBEXECDIR = `apxs -q LIBEXECDIR`

$install = CONFIG["INSTALL_PROGRAM"]
$install_data = CONFIG["INSTALL_DATA"]
$install_dllib = CONFIG["INSTALL_DLLIB"] || $install_data
if $install =~ %r!^[^\s/]+/! then
  $install = CONFIG["compile_dir"]+"/"+$install
  $install_dllib = CONFIG["compile_dir"]+"/"+$install_dllib
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

CFLAGS = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} -I#{$APACHE_INCLUDEDIR} \\
	 #{$CFLAGS}
LIBS = #{CONFIG["LIBS"]}
XLDFLAGS = #{CONFIG["XLDFLAGS"]}
LDSHARED = #{CONFIG["LDSHARED"]}
DLDFLAGS = #{CONFIG["DLDFLAGS"]}
INSTALL = #{$install}
INSTALL_DLLIB = #{$install_dllib}

RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
LIBRUBYARG = #$LIBRUBYARG
OBJS = mod_ruby.o ruby_config.o apachelib.o eruby.o
TARGET = mod_ruby.so

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

install: $(TARGET)
	$(INSTALL_DLLIB) $(TARGET) #{$APACHE_LIBEXECDIR}

clean:
	rm -f $(TARGET) $(OBJS) *~

distclean: clean
	rm -f Makefile

$(TARGET): $(OBJS)
	$(LDSHARED) $(DLDFLAGS) $(XLDFLAGS) -o $(TARGET) $(OBJS) $(LIBRUBYARG) $(LIBS)

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
