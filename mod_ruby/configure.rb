#!/usr/bin/env ruby

require "mkmf"
require "getoptlong"

def usage
  $stderr.print(<<EOS)
usage: configure.rb [options]

  --help                      print this message

  --prefix=PREFIX             install architecture-independent files in PREFIX
                              [#{CONFIG["prefix"]}]
  --exec-prefix=EPREFIX       install architecture-dependent files in EPREFIX
                              [same as prefix]
  --bindir=DIR                user executables in DIR [EPREFIX/bin]
  --libdir=DIR                object code libraries in DIR [EPREFIX/lib]
  --includedir=DIR            C header files in DIR [PREFIX/include]
  --mandir=DIR                manual pages in DIR [PREFIX/man]

  --apxs=APXS                 path to apxs command
  --enable-eruby              enable eruby support
  --eruby-includes=DIR        eruby include files are in DIR
  --eruby-libraries=DIR       eruby library files are in DIR
EOS
end

def create_file(filename, config)
  print "creating ", filename, "\n"
  open(filename + ".in") do |fin|
    open(filename, "w") do |fout|
      while line = fin.gets
	line.gsub!(/@[A-Za-z_]+@/) do |s|
	  key = s[1..-2]
	  if config.key?(key)
	    config[key]
	  else
	    s
	  end
	end
	fout.print(line)
      end
    end
  end
end

$APXS = "apxs"
$ENABLE_ERUBY = false
$ERUBY_INCLUDES = nil
$ERUBY_LIBRARIES = nil

drive = File::PATH_SEPARATOR == ';' ? /\A\w:/ : /\A/
prefix = Regexp.new("\\A" + Regexp.quote(CONFIG["prefix"]))
$prefix = CONFIG["prefix"].sub(drive, '')
$exec_prefix = "$(prefix)"
$bindir = CONFIG["bindir"].sub(prefix, "$(exec_prefix)").sub(drive, '')
$libdir = CONFIG["libdir"].sub(prefix, "$(exec_prefix)").sub(drive, '')
$archdir = $archdir.sub(prefix, "$(prefix)").sub(drive, '')
$sitelibdir = $sitelibdir.sub(prefix, "$(prefix)").sub(drive, '')
$sitearchdir = $sitearchdir.sub(prefix, "$(prefix)").sub(drive, '')
$includedir = CONFIG["includedir"].sub(prefix, "$(prefix)").sub(drive, '')
$mandir = CONFIG["mandir"].sub(prefix, "$(prefix)").sub(drive, '')

parser = GetoptLong.new
parser.set_options(["--help", GetoptLong::NO_ARGUMENT],
		   ["--prefix", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--exec-prefix", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--bindir", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--libdir", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--includedir", GetoptLong::OPTIONAL_ARGUMENT],
		   ["--mandir", GetoptLong::OPTIONAL_ARGUMENT],
		   ['--apxs', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--enable-eruby', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--eruby-includes', GetoptLong::OPTIONAL_ARGUMENT],
		   ['--eruby-libraries', GetoptLong::OPTIONAL_ARGUMENT])

begin
  parser.each_option do |name, arg|
    case name
    when "--prefix"
      $prefix = arg
    when "--exec-prefix"
      $exec_prefix = arg
    when "--bindir"
      $bindir = arg
    when "--libdir"
      $libdir = arg
    when "--includedir"
      $includedir = arg
    when "--mandir"
      $mandir = arg
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

config = {}

config["srcdir"] = File.dirname($0)
config["topdir"] = $topdir
config["hdrdir"] = $hdrdir
config["VPATH"] = ""

config["RUBY_INSTALL_NAME"] = CONFIG["RUBY_INSTALL_NAME"]
config["arch"] = CONFIG["arch"]
config["ruby_version"] = Config::CONFIG["ruby_version"]

config["prefix"] = $prefix
config["exec_prefix"] = $exec_prefix

config["bindir"] = $bindir
config["libdir"] = $libdir
config["rubylibdir"] = $rubylibdir ||
  $libdir + "/ruby/" + CONFIG["MAJOR"] + "." + CONFIG["MINOR"]
config["archdir"] = $archdir
config["sitedir"] = $sitedir
config["sitelibdir"] = $sitelibdir
config["sitearchdir"] = $sitearchdir
config["includedir"] = $includedir
config["mandir"] = $mandir

config["CC"] = CONFIG["CC"]
config["AR"] = CONFIG["AR"]
config["AROPT"] = "rcu $@"
config["LD"] = "$(CC)"
config["RANLIB"] = CONFIG["RANLIB"]

config["CFLAGS"] = CFLAGS + " " + CONFIG["CCDLFLAGS"]
config["LDFLAGS"] = CONFIG["LDFLAGS"]
config["LIBS"] = CONFIG["LIBS"]
config["XLDFLAGS"] = $XLDFLAGS
config["DLDFLAGS"] = $DLDFLAGS
config["LDSHARED"] = CONFIG["LDSHARED"]

config["OBJEXT"] = $OBJEXT

begin
  config["LIBRUBYARG"] = Config.expand(CONFIG["LIBRUBYARG"])
rescue
  config["LIBRUBYARG"] = CONFIG["LIBRUBYARG"]
end
if config["LIBRUBYARG"] =~ /\.a$/
  $RUBY_SHARED = false
  config["LIBRUBYARG"] = $hdrdir + "/" + config["LIBRUBYARG"]
else
  $RUBY_SHARED = true
  config["LIBRUBYARG"].gsub!(/-L\./, "-L#{CONFIG['prefix']}/lib")
end

case PLATFORM
when /-aix/
  config["DLDFLAGS"] = "-Wl,-bE:mod_ruby.imp -Wl,-bI:httpd.exp -Wl,-bM:SRE -Wl,-bnoentry -lc"
  if $RUBY_SHARED
    config["LIBRUBYARG"] = "-Wl," + CONFIG["libdir"] + "/" + CONFIG["LIBRUBY_SO"]
    config["LIBRUBYARG"].sub!(/\.so\.[.0-9]*$/, '.so')
    config["XLDFLAGS"] = ""
  else
    config["XLDFLAGS"] = "-Wl,-bE:#{$topdir}/ruby.imp"
  end
  if CONFIG["DLDFLAGS"] !~ /-Wl,/
    config["LIBRUBYARG"].gsub!(/-Wl,/, '')
    config["XLDFLAGS"].gsub!(/-Wl,/, '')
    config["DLDFLAGS"].gsub!(/-Wl,/, '')
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
end

config["CFLAGS"] += " " + `#{$APXS} -q CFLAGS`
config["APACHE_INCLUDEDIR"] = `#{$APXS} -q INCLUDEDIR`
config["APACHE_LIBEXECDIR"] = `#{$APXS} -q LIBEXECDIR`

if $ENABLE_ERUBY
  if $ERUBY_INCLUDES
    config["CFLAGS"] += " -DUSE_ERUBY -I#{$ERUBY_INCLUDES}"
  else
    config["CFLAGS"] += " -DUSE_ERUBY"
  end
  if $ERUBY_LIBRARIES
    config["LIBERUBYARG"] = "-L#{$ERUBY_LIBRARIES} -leruby"
  else
    config["LIBERUBYARG"] = "-leruby"
  end
else
  config["LIBERUBYARG"] = ""
end

create_file("Makefile", config)

# Local variables:
# mode: Ruby
# tab-width: 8
# End:
