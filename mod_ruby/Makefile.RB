#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

$APXS = "apxs"
$ENABLE_ERUBY = false
$ERUBY_INCLUDES = nil
$ERUBY_LIBRARIES = nil

parser = GetoptLong.new
parser.set_options(['--help', GetoptLong::NO_ARGUMENT],
		   ['--apxs', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--enable-eruby', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--eruby-includes', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--eruby-libraries', GetoptLong::OPTIONAL_ARGUMENT])

def usage
  $stderr.printf <<EOS, $0
usage: %s [options]

  --help                        print this message
  --apxs=APXS                   path to apxs command
  --enable-eruby                enable eruby support
  --eruby-includes=DIR		eruby include files are in DIR
  --eruby-libraries=DIR		eruby library files are in DIR
EOS
end

begin
  parser.each_option do |name, arg|
    case name
    when "--apxs"
      $APXS = arg
    when "--enable-eruby"
      $ENABLE_ERUBY = true
    when "--eruby-includes"
      $ERUBY_INCLUDES = arg
    when "--eruby-libraries"
      $ERUBY_LIBRARIES = arg
    when "--help"
      usage
      exit(1)
    end
  end
rescue
  usage
  exit(1)
end

begin
  $LIBRUBYARG = Config.expand(CONFIG["LIBRUBYARG"])
rescue
  $LIBRUBYARG = CONFIG["LIBRUBYARG"]
end
if $LIBRUBYARG =~ /\.a$/
  $shared = false
  $LIBRUBYARG = $hdrdir + "/" + $LIBRUBYARG
else
  $shared = true
  $LIBRUBYARG.gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

case PLATFORM
when /-aix/
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
#'
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

$ERUBY_CFLAGS = ""
$LIBERUBYARG = ""
if $ENABLE_ERUBY
  if $ERUBY_INCLUDES
    $ERUBY_CFLAGS = "-DUSE_ERUBY -I#{$ERUBY_INCLUDES}"
  else
    $ERUBY_CFLAGS = "-DUSE_ERUBY"
  end
  if $ERUBY_LIBRARIES
    $LIBERUBYARG = "-L#{$ERUBY_LIBRARIES} -leruby"
  else
    $LIBERUBYARG = "-leruby"
  end
end

mfile = open("Makefile", "w")
begin
  mfile.write <<EOMF
# Generated automatically by Makefile.RB.

CC = #{CONFIG["CC"]}
RUBY_INSTALL_NAME = #{CONFIG["RUBY_INSTALL_NAME"]}
RUBY = $(RUBY_INSTALL_NAME)

CFLAGS = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} -I#{$APACHE_INCLUDEDIR} \\
  #{$ERUBY_CFLAGS} #{$CFLAGS}
LIBS = #{CONFIG["LIBS"]}
XLDFLAGS = #{$XLDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}
DLDFLAGS = #{$DLDFLAGS}
INSTALL_DLLIB = $(RUBY) -r ftools -e 'File.install ARGV[0], ARGV[1], 0555, true'
LIBRUBYARG = #{$LIBRUBYARG}
LIBERUBYARG = #{$LIBERUBYARG}

TARGET = mod_ruby.so

OBJS = mod_ruby.o ruby_config.o apachelib.o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

install: $(TARGET)
	$(INSTALL_DLLIB) $(TARGET) #{$APACHE_LIBEXECDIR}

clean:
	rm -f $(TARGET) $(OBJS) *~

distclean: clean
	rm -f mod_ruby.imp Makefile

$(TARGET): $(OBJS)
	$(LDSHARED) $(DLDFLAGS) $(XLDFLAGS) -o $(TARGET) $(OBJS) \
		$(LIBERUBYARG) $(LIBRUBYARG) $(LIBS)

mod_ruby.o: mod_ruby.c mod_ruby.h ruby_config.h apachelib.h
ruby_config.o: ruby_config.c mod_ruby.h ruby_config.h
apachelib.o: apachelib.c apachelib.h
EOMF
ensure
  mfile.close
end

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
