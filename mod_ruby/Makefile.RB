#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

$APXS = "apxs"
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

parser = GetoptLong.new
parser.set_options(['--help', GetoptLong::NO_ARGUMENT],
		   ['--apxs', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--default-charset', GetoptLong::OPTIONAL_ARGUMENT])

def usage
  $stderr.printf <<EOS, $0
usage: %s [options]

  --help                        print this message
  --apxs=APXS                   path to apxs command
  --default-charset=CHARSET     default charset value
EOS
end

begin
  parser.each_option do |name, arg|
    case name
    when "--apxs"
      $APXS = arg
    when "--default-charset"
      $DEFAULT_CHARSET = arg
    when "--help"
      usage
      exit(1)
    end
  end
rescue
  usage
  exit(1)
end

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
    $LIBRUBYARG = "-Wl," + CONFIG["libdir"] + "/" + CONFIG["LIBRUBY_SO"]
    $LIBRUBYARG.sub!(/\.so\.[.0-9]*$/, '.so')
    $XLDFLAGS = ""
  else
    $XLDFLAGS = "-Wl,-bE:#{$topdir}/ruby.imp"
  end
  if CONFIG["DLDFLAGS"] !~ /-Wl,/
    $LIBRUBYARG.gsub!(/-Wl,/, '')
    $XLDFLAGS.gsub!(/-Wl,/, '')
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
  $XLDFLAGS = CONFIG["XLDFLAGS"]
  $DLDFLAGS = CONFIG["DLDFLAGS"]
end

$APACHE_CFLAGS = `#{$APXS} -q CFLAGS`
if $APACHE_CFLAGS.size > 0
  $CFLAGS = ($APACHE_CFLAGS.split(/[ \t]+/) | CFLAGS.split(/[ \t]+/)).join(" ")
else
  $CFLAGS = CFLAGS
end
$APACHE_INCLUDEDIR = `#{$APXS} -q INCLUDEDIR`
$APACHE_LIBEXECDIR = `#{$APXS} -q LIBEXECDIR`

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}

CFLAGS = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} -I#{$APACHE_INCLUDEDIR} \\
	 #{$CFLAGS}
LIBS = #{CONFIG["LIBS"]}
XLDFLAGS = #$XLDFLAGS
LDSHARED = #{CONFIG["LDSHARED"]}
DLDFLAGS = #$DLDFLAGS
INSTALL_DLLIB = ruby -r ftools -e 'File.install ARGV[0], ARGV[1], 0555, true'

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
	rm -f config.h $(TARGET) $(OBJS) *~

distclean: clean
	rm -f mod_ruby.imp Makefile

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

cfile = open("config.h", "w")
begin
  cfile.write <<EOMF
/* Generated automatically by Makefile.RB. */

#define ERUBY_DEFAULT_CHARSET "#{$DEFAULT_CHARSET}"
EOMF
ensure
  cfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
