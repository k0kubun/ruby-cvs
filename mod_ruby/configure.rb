#!/usr/bin/env ruby

# Generated automatically using autoconf.rb version 0.1

require "mkmf"

$ac_help = ""
$ac_help += "  --enable-eruby          enable eruby" "\n"
$ac_help += "  --with-apache=DIR       Build static Apache module.  DIR is the path
                          to the top-level Apache source directory" "\n"
$ac_help += "  --with-apxs[=FILE]      Build shared Apache module.  FILE is the optional
                          pathname to the Apache apxs tool; [apxs]" "\n"
$ac_help += "  --with-eruby-includes=DIR   eruby include files are in DIR" "\n"
$ac_help += "  --with-eruby-libraries=DIR  eruby library files are in DIR" "\n"

$ac_sed = {}
$ac_confdefs = {}
$ac_features = {}
$ac_packages = {}
  
def AC_SUBST(variable)
  $ac_sed[variable] = true
end

def AC_DEFINE(variable, value = 1)
  case value
  when true
    value = "1"
  when false
    value = "0"
  when String
    value = value.dump
  else
    value = value.inspect
  end
  $ac_confdefs[variable] = value
end

AC_GIVEN = Object.new
def AC_GIVEN.if_not_given
  # do nothing
end

AC_NOT_GIVEN = Object.new
def AC_NOT_GIVEN.if_not_given
  yield
end

def AC_ENABLE(feature, action = Proc.new)
  if $ac_features[feature]
    yield($ac_features[feature])
    return AC_GIVEN
  else
    return AC_NOT_GIVEN
  end
end

def AC_WITH(package, action = Proc.new)
  if $ac_packages[package]
    yield($ac_packages[package])
    return AC_GIVEN
  else
    return AC_NOT_GIVEN
  end
end

def AC_OUTPUT(*files)
  if $AC_LIST_HEADER
    $DEFS = "-DHAVE_CONFIG_H"
    AC_OUTPUT_HEADER($AC_LIST_HEADER)
  else
    $DEFS = $ac_confdefs.collect {|k, v| "-D#{k}=#{v}" }.join(" ")
  end
  for file in files
    print "creating ", file, "\n"
    open(File.join($srcdir, file + ".in")) do |fin|
      open(file, "w") do |fout|
	while line = fin.gets
	  line.gsub!(/@([A-Za-z_]+)@/) do |s|
	    name = $1
	    if $ac_sed.key?(name)
	      eval("$" + name) #"
	    else
	      s
	    end
	  end
	  fout.print(line)
	end
      end
    end
  end
end

def AC_OUTPUT_HEADER(header)
  print "creating ", header, "\n"
  open(File.join($srcdir, header + ".in")) do |fin|
    open(header, "w") do |fout|
      while line = fin.gets
	line.sub!(/^(#define|#undef)\s+([A-Za-z_]+).*$/) do |s|
	  name = $2
	  if $ac_confdefs.key?(name)
	    val = $ac_confdefs[name]
	    "#define #{name} #{val}"
	  else
	    s
	  end
	end
	fout.print(line)
      end
    end
  end
  $ac_confdefs.clear
end

def AC_CONFIG_HEADER(header)
  $AC_LIST_HEADER = header
end

def AC_MSG_CHECKING(feature)
  print "checking #{feature}... "
  $stdout.flush
end

def AC_MSG_RESULT(result)
  case result
  when true
    result = "yes"
  when false, nil
    result = "no"
  end
  puts(result)
end

def AC_MSG_WARN(msg)
  $stderr.print("configure.rb: warning: ", msg, "\n")
end

def AC_MSG_ERROR(msg)
  $stderr.print("configure.rb: error: ", msg, "\n")
  exit(1)
end

$stdout.sync = true

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

$rubylibdir ||=
  $libdir + "/ruby/" + CONFIG["MAJOR"] + "." + CONFIG["MINOR"]

for option in ARGV
  if option =~ /^-.*?=(.*)/
    optarg = $1
  else
    optarg = nil
  end
  case option
  when /^--prefix/
    $prefix = optarg
  when /^--exec-prefix/
    $exec_prefix = optarg
  when /^--bindir/
    $bindir = optarg
  when /^--libdir/
    $libdir = optarg
  when /^--includedir/
    $includedir = optarg
  when /^--mandir/
    $mandir = optarg
  when /^--enable-([^=]+)/
    feature = $1
    if optarg.nil?
      optarg = "yes"
    end
    $ac_features[feature] = optarg
  when /^--with-([^=]+)/
    package = $1
    if optarg.nil?
      optarg = "yes"
    end
    $ac_packages[package] = optarg
  when "--help"
    print <<EOF
Usage: configure.rb [options]
Options: [defaults in brackets after descriptions]
Configuration:
  --help                  print this message
Directory and file names:
  --prefix=PREFIX         install architecture-independent files in PREFIX
                          [#{$prefix}]
  --exec-prefix=EPREFIX   install architecture-dependent files in EPREFIX
                          [same as prefix]
  --bindir=DIR            user executables in DIR [EPREFIX/bin]
  --libdir=DIR            object code libraries in DIR [EPREFIX/lib]
  --includedir=DIR        C header files in DIR [PREFIX/include]
  --mandir=DIR            manual pages in DIR [PREFIX/man]
EOF
    if $ac_help.length > 0
      print "--enable and --with options recognized:\n"
      print $ac_help
    end
    exit(0)
  when /^-.*/
    AC_MSG_ERROR("#{option}: invalid option; use --help to show usage")
  end
end

$srcdir = File.dirname($0)
$VPATH = ""

$RUBY_INSTALL_NAME = CONFIG["RUBY_INSTALL_NAME"]
$arch = CONFIG["arch"]
$ruby_version = Config::CONFIG["ruby_version"] ||
  CONFIG["MAJOR"] + "." + CONFIG["MINOR"]

$CC = CONFIG["CC"]
$AR = CONFIG["AR"]
$LD = "$(CC)"
$RANLIB = CONFIG["RANLIB"]

$CFLAGS = CFLAGS + " " + CONFIG["CCDLFLAGS"]
$LDFLAGS = CONFIG["LDFLAGS"]
$LIBS = CONFIG["LIBS"]
$XLDFLAGS = CONFIG["XLDFLAGS"]
$DLDFLAGS = CONFIG["DLDFLAGS"]
$LDSHARED = CONFIG["LDSHARED"]

$EXEEXT = CONFIG["EXEEXT"]
$DLEXT = CONFIG["DLEXT"]

$LIBRUBYARG = CONFIG["LIBRUBYARG"]
if $LIBRUBYARG =~ /\.a$/
  $RUBY_SHARED = false
  $LIBRUBYARG = "$(hdrdir)/" + $LIBRUBYARG
else
  $RUBY_SHARED = true
  $LIBRUBYARG.gsub!(/-L\./, "-L$(libdir)")
end

case PLATFORM
when /-aix/
  if $RUBY_SHARED
    $LIBRUBYARG = "-Wl,$(libdir)/" + CONFIG["LIBRUBY_SO"]
    $LIBRUBYARG.sub!(/\.so\.[.0-9]*$/, '.so')
    $XLDFLAGS = ""
  else
    $XLDFLAGS = "-Wl,-bE:$(topdir)/ruby.imp"
  end
  if $DLDFLAGS !~ /-Wl,/
    $LIBRUBYARG.gsub!(/-Wl,/, '')
    $XLDFLAGS.gsub!(/-Wl,/, '')
    $DLDFLAGS.gsub!(/-Wl,/, '')
  end
end

AC_SUBST("srcdir")
AC_SUBST("topdir")
AC_SUBST("hdrdir")
AC_SUBST("VPATH")

AC_SUBST("RUBY_INSTALL_NAME")
AC_SUBST("arch")
AC_SUBST("ruby_version")
AC_SUBST("prefix")
AC_SUBST("exec_prefix")
AC_SUBST("bindir")
AC_SUBST("libdir")
AC_SUBST("rubylibdir")
AC_SUBST("archdir")
AC_SUBST("sitedir")
AC_SUBST("sitelibdir")
AC_SUBST("sitearchdir")
AC_SUBST("includedir")
AC_SUBST("mandir")

AC_SUBST("CC")
AC_SUBST("AR")
AC_SUBST("LD")
AC_SUBST("RANLIB")

AC_SUBST("CFLAGS")
AC_SUBST("DEFS")
AC_SUBST("LDFLAGS")
AC_SUBST("LIBS")
AC_SUBST("XLDFLAGS")
AC_SUBST("DLDFLAGS")
AC_SUBST("LDSHARED")

AC_SUBST("OBJEXT")
AC_SUBST("EXEEXT")
AC_SUBST("DLEXT")

AC_SUBST("LIBRUBYARG")

AC_MSG_CHECKING("Ruby version")
AC_MSG_RESULT(RUBY_VERSION)
if RUBY_VERSION < "1.6"
  AC_MSG_ERROR("mod_ruby requires Ruby 1.6.x or later.")
end
if RUBY_VERSION < "1.6.4"
  AC_MSG_WARN("mod_ruby recommends Ruby 1.6.4 or later.")
end

AC_MSG_CHECKING("for static Apache module support")
AC_WITH("apache") { |withval|
  if withval == "yes"
    AC_MSG_ERROR("You need to specify a directory with --with-apache")
  end
  unless File.file?("#{withval}/src/include/httpd.h")
    AC_MSG_ERROR("Unable to locate #{withval}/src/include/httpd.h")
  end
  $APACHE_SRCDIR = withval
  $APACHE_INCLUDES = "-I#{withval}/src/include -I#{withval}/src/os/unix"
  $TARGET = "libruby.a"
  $install = "install-static"
  AC_MSG_RESULT("yes")
}.if_not_given {
  AC_MSG_RESULT("no")
}

AC_MSG_CHECKING("for dynamic Apache module support")
AC_WITH("apxs") { |withval|
  if withval == "yes"
    $APXS = "apxs"
  else
    $APXS = withval
  end
}.if_not_given {
  unless $TARGET
    $APXS = "apxs"
  end
}
AC_MSG_RESULT($APXS)

if $APXS
  $CFLAGS += " " + `#{$APXS} -q CFLAGS`
  $APACHE_INCLUDES = "-I" + `#{$APXS} -q INCLUDEDIR`
  $APACHE_LIBEXECDIR = `#{$APXS} -q LIBEXECDIR`
  $TARGET = "mod_ruby.so"
  $install = "install-shared"
end

AC_SUBST("TARGET")
AC_SUBST("install")
AC_SUBST("APACHE_SRCDIR")
AC_SUBST("APACHE_INCLUDES")
AC_SUBST("APACHE_LIBEXECDIR")

case PLATFORM
when /-aix/
  $DLDFLAGS = "-Wl,-bE:mod_ruby.imp -Wl,-bI:httpd.exp -Wl,-bM:SRE -Wl,-bnoentry -lc"
  open("mod_ruby.imp", "w") do |ifile|
    ifile.write <<EOF
#!
ruby_module
EOF
  end
  print <<EOF
To build mod_ruby on the AIX platform, you need to have the apache
export file `httpd.exp' in the current directory.
Please copy <apache-src-directory>/support/httpd.exp to the current
directory before making `mod_ruby.so'.
EOF
#'
end

AC_MSG_CHECKING("whether enable eruby")
$ENABLE_ERUBY = false
AC_ENABLE("eruby") { |enableval|
  if enableval == "yes"
    $ENABLE_ERUBY = true
    $LIBERUBYARG = "-leruby"
    AC_DEFINE("USE_ERUBY")
  end
}
AC_MSG_RESULT($ENABLE_ERUBY)

AC_WITH("eruby-includes") { |withval|
  if $ENABLE_ERUBY
    $CFLAGS += " -I#{withval}"
  end
}

AC_WITH("eruby-libraries") { |withval|
  if $ENABLE_ERUBY
    $LIBERUBYARG = "-L#{withval} #{$LIBERUBYARG}"
  end
}

AC_SUBST("LIBERUBYARG")

$MODULE_LIBS = "#{$LIBERUBYARG} #{Config.expand($LIBRUBYARG)} #{$LIBS}"

AC_OUTPUT("Makefile", "libruby.module")
