# module to create Makefile for extention modules
# invoke like: ruby -r mkmf extconf.rb

require 'rbconfig'

include Config

$found = false;
$lib_cache = {}
$lib_found = {}
$func_cache = {}
$func_found = {}
$hdr_cache = {}
$hdr_found = {}

$config_cache = CONFIG["compile_dir"]+"/ext/config.cache"
if File.exist?($config_cache) then
  f = open($config_cache, "r")
  while f.gets
    case $_
    when /^lib: (.+) (yes|no)/
      $lib_cache[$1] = $2
    when /^func: ([\w_]+) (yes|no)/
      $func_cache[$1] = $2
    when /^hdr: (.+) (yes|no)/
      $hdr_cache[$1] = $2
    end
  end
  f.close
end

$srcdir = CONFIG["srcdir"]
$libdir = CONFIG["libdir"]+"/"+CONFIG["ruby_install_name"]
$archdir = $libdir+"/"+CONFIG["arch"]
$install = CONFIG["INSTALL_PROGRAM"]
$install_data = CONFIG["INSTALL_DATA"]
if $install !~ /^\// then
  $install = CONFIG["srcdir"]+"/"+$install
end

if File.exist? $archdir + "/ruby.h"
  $hdrdir = $archdir
elsif File.exist? $srcdir + "/ruby.h"
  $hdrdir = $srcdir
else
  STDERR.print "can't find header files for ruby.\n"
  exit 1
end

nul = "> /dev/null"

CFLAGS = CONFIG["CFLAGS"]
if PLATFORM == "m68k-human"
  nul = "> nul"
  CFLAGS.gsub!(/-c..-stack=[0-9]+ */, '')
end
if $DEBUG
  nul = ""
end
LINK = CONFIG["CC"]+" -o conftest -I#{$srcdir} " + CFLAGS + " %s " + CONFIG["LDFLAGS"] + " %s conftest.c " + CONFIG["LIBS"] + "%s " + nul + " 2>&1"
CPP = CONFIG["CPP"] + " -E  -I#{$srcdir} " + CFLAGS + " %s conftest.c " + nul + " 2>&1"

def try_link(libs)
  system(format(LINK, $CFLAGS, $LDFLAGS, libs))
end

def try_cpp
  system(format(CPP, $CFLAGS))
end

def have_library(lib, func)
  printf "checking for %s() in -l%s... ", func, lib
  STDOUT.flush
  if $lib_cache[lib]
    if $lib_cache[lib] == "yes"
      if $libs
        $libs = "-l" + lib + " " + $libs 
      else
	$libs = "-l" + lib
      end
      print "(cached) yes\n"
      return TRUE
    else
      print "(cached) no\n"
      return FALSE
    end
  end

  cfile = open("conftest.c", "w")
  cfile.printf "\
int main() { return 0; }
int t() { %s(); return 0; }
", func
  cfile.close

  begin
    if $libs
      libs = "-l" + lib + " " + $libs 
    else
      libs = "-l" + lib
    end
    unless try_link(libs)
      $lib_found[lib] = 'no'
      $found = TRUE
      print "no\n"
      return FALSE
    end
  ensure
    system "rm -f conftest*"
  end

  $libs = libs
  $lib_found[lib] = 'yes'
  $found = TRUE
  print "yes\n"
  return TRUE
end

def have_func(func)
  printf "checking for %s()... ", func
  STDOUT.flush
  if $func_cache[func]
    if $func_cache[func] == "yes"
      $defs.push(format("-DHAVE_%s", func.upcase))
      print "(cached) yes\n"
      return TRUE
    else
      print "(cached) no\n"
      return FALSE
    end
  end

  cfile = open("conftest.c", "w")
  cfile.printf "\
char %s();
int main() { return 0; }
int t() { %s(); return 0; }
", func, func
  cfile.close

  libs = $libs
  libs = "" if libs == nil

  begin
    unless try_link(libs)
      $func_found[func] = 'no'
      $found = TRUE
      print "no\n"
      return FALSE
    end
  ensure
    system "rm -f conftest*"
  end
  $defs.push(format("-DHAVE_%s", func.upcase))
  $func_found[func] = 'yes'
  $found = TRUE
  print "yes\n"
  return TRUE
end

def have_header(header)
  printf "checking for %s... ", header
  STDOUT.flush
  if $hdr_cache[header]
    if $hdr_cache[header] == "yes"
      header.tr!("a-z./\055", "A-Z___")
      $defs.push(format("-DHAVE_%s", header))
      print "(cached) yes\n"
      return TRUE
    else
      print "(cached) no\n"
      return FALSE
    end
  end

  cfile = open("conftest.c", "w")
  cfile.printf "\
#include <%s>
", header
  cfile.close

  begin
    unless try_cpp
      $hdr_found[header] = 'no'
      $found = TRUE
      print "no\n"
      return FALSE
    end
  ensure
    system "rm -f conftest*"
  end
  $hdr_found[header] = 'yes'
  header.tr!("a-z./\055", "A-Z___")
  $defs.push(format("-DHAVE_%s", header))
  $found = TRUE
  print "yes\n"
  return TRUE
end

def create_header()
  print "creating extconf.h\n"
  STDOUT.flush
  if $defs.length > 0
    hfile = open("extconf.h", "w")
    for line in $defs
      line =~ /^-D(.*)/
      hfile.printf "#define %s 1\n", $1
    end
    hfile.close
  end
end

def create_makefile(target)
  print "creating Makefile\n"
  STDOUT.flush
  if $libs and CONFIG["DLEXT"] == "o"
    libs = $libs.split
    for lib in libs
      lib.sub!(/-l(.*)/, '"lib\1.a"')
    end
    $defs.push(format("-DEXTLIB='%s'", libs.join(",")))
  end
  $libs = "" unless $libs

  if !$objs then
    $objs = Dir["*.c"]
    for f in $objs
      f.sub!(/\.(c|cc)$/, ".o")
    end
  end
  $objs = $objs.join(" ")

  mfile = open("Makefile", "w")
  mfile.print  <<EOMF
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = #{$srcdir}
hdrdir = #{$hdrdir}

CC = gcc

CFLAGS   = #{CONFIG["CCDLFLAGS"]} -I#{$hdrdir} #{CFLAGS} #{$CFLAGS} #{$defs.join(" ")}
DLDFLAGS = #{CONFIG["DLDFLAGS"]} #{$LDFLAGS}
LDSHARED = #{CONFIG["LDSHARED"]}

prefix = #{CONFIG["prefix"]}
exec_prefix = #{CONFIG["exec_prefix"]}
libdir = #{$archdir}

#### End of system configuration section. ####

LOCAL_LIBS = #{$local_libs}
LIBS = #{$libs}
OBJS = #{$objs}

TARGET = #{target}.#{CONFIG["DLEXT"]}

INSTALL = #{$install}

binsuffix = #{CONFIG["binsuffix"]}

all:		$(TARGET)

clean:;		@rm -f *.o *.so *.sl
		@rm -f Makefile extconf.h conftest.*
		@rm -f core ruby$(binsuffix) *~

realclean:	clean

install:	$(libdir)/$(TARGET)

$(libdir)/$(TARGET): $(TARGET)
	@test -d $(libdir) || mkdir $(libdir)
	$(INSTALL) $(TARGET) $(libdir)/$(TARGET)
EOMF
  for rb in Dir["lib/*.rb"]
    mfile.printf "\t$(INSTALL) %s %s\n", rb, $libdir
  end
  mfile.printf "\n"

  if CONFIG["DLEXT"] != "o"
    mfile.printf <<EOMF
$(TARGET): $(OBJS)
	$(LDSHARED) $(DLDFLAGS) -o $(TARGET) $(OBJS) $(LOCAL_LIBS) $(LIBS)
EOMF
  elsif not File.exist?(target + ".c") and not File.exist?(target + ".cc") or 
    mfile.print "$(TARGET): $(OBJS)\n"
    case PLATFORM
    when "m68k-human"
      mfile.printf "ar cru $(TARGET) $(OBJS)\n"
    when /-nextstep/
      mfile.printf "cc -r $(CFLAGS) -o $(TARGET) $(OBJS)\n"
    else
      mfile.printf "ld $(DLDFLAGS) -r -o $(TARGET) $(OBJS)\n"
    end
  end

  if File.exist?("depend")
    dfile = open("depend", "r")
    mfile.printf "###\n"
    while line = dfile.gets()
      mfile.print line
    end
    dfile.close
  end
  mfile.close

  if $found
    begin
      f = open($config_cache, "w")
      for k,v in $lib_cache
	f.printf "lib: %s %s\n", k, v.downcase
      end
      for k,v in $lib_found
	f.printf "lib: %s %s\n", k, v.downcase
      end
      for k,v in $func_cache
	f.printf "func: %s %s\n", k, v.downcase
      end
      for k,v in $func_found
	f.printf "func: %s %s\n", k, v.downcase
      end
      for k,v in $hdr_cache
	f.printf "hdr: %s %s\n", k, v.downcase
      end
      for k,v in $hdr_found
	f.printf "hdr: %s %s\n", k, v.downcase
      end
      f.close
    rescue
    end
  end
end

$local_libs = nil
$libs = nil
$objs = nil
$CFLAGS = nil
$LDFLAGS = nil
$defs = []

