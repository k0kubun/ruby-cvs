#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

ENV["PATH"] += ":/sbin:/usr/sbin:/usr/local/sbin:\
/usr/apache/bin:/usr/local/apache/bin"

$USE_ERUBY = false
$ERUBY_INCLUDEDIR = "../eruby"

opts = GetoptLong.new(["--enable-eruby", GetoptLong::NO_ARGUMENT],
		      ["--eruby-include-dir", GetoptLong::REQUIRED_ARGUMENT],
		      ["--help", GetoptLong::NO_ARGUMENT],)
begin
  for name, arg in opts
    case name
    when "--enable-eruby"
      $USE_ERUBY = true
    when "--eruby-include-dir"
      $ERUBY_INCLUDEDIR = arg
    when "--help"
      $stderr.print <<EOF
Usage: #{$0} [options]
Configuration:
  --help			print this message

  --enable-eruby		enable eruby
  --eruby-include-dir=DIR	eruby in DIR
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

ERUBY_INCLUDEDIR = #{$ERUBY_INCLUDEDIR}
LIBERUBY = #{ $USE_ERUBY ? "-leruby" : "" }

CFLAGS = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} -I#{$APACHE_INCLUDEDIR} \\
	 #{$CFLAGS} #{ $USE_ERUBY ? "-I$(ERUBY_INCLUDEDIR) -DUSE_ERUBY" : "" }
LDSHARED = #{CONFIG["LDSHARED"]}
DLDFLAGS = #{CONFIG["DLDFLAGS"]}
INSTALL = #{CONFIG["INSTALL"]}
INSTALL_DATA = #{CONFIG["INSTALL_DATA"]}

RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
LIBRUBYARG = #$LIBRUBYARG
OBJS = ruby_module.o ruby_config.o apachelib.o
TARGET = mod_ruby.#{CONFIG["DLEXT"]}

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
	$(LDSHARED) $(DLDFLAGS) -o $(TARGET) $(OBJS) $(LIBERUBY) $(LIBRUBYARG)

ruby_module.o: ruby_module.c ruby_module.h ruby_config.h apachelib.h
ruby_config.o: ruby_config.c ruby_module.h ruby_config.h
apachelib.o: apachelib.c apachelib.h
EOMF
ensure
  mfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
