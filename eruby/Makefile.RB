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

parser = GetoptLong.new
parser.set_options(['--help', GetoptLong::NO_ARGUMENT],
		   ['--default-charset', GetoptLong::OPTIONAL_ARGUMENT])

def usage
  $stderr.printf <<EOS, $0
usage: %s [options]
				
  --help                        print this message
  --default-charset=CHARSET     default charset value
EOS
end

begin
  parser.each_option do |name, arg|
    case name
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
	strip $(TARGET)
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)/$(TARGET)

clean:
	rm -f config.h $(TARGET) $(OBJS) *~

distclean: clean
	rm -f Makefile

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(XLDFLAGS) $(OBJS) $(LIBRUBYARG) $(LIBS) -o $@

eruby_logo.c: eruby_logo.png
	./bin2c eruby_logo.png
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
