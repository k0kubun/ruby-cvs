#!./miniruby

load "./rbconfig.rb"
include Config

File.umask(0)

while arg = ARGV.shift
  case arg
  when /^--/			# ignore
  when /^-/
    $dryrun = /n/ =~ arg
  when /=/			# ignore
  else
    destdir ||= arg
    break
  end
end
destdir ||= ''

$:.unshift CONFIG["srcdir"]+"/lib"
require 'ftools'

class Installer < File; end
class << Installer
  if $dryrun
    def makedirs(*dirs)
      String === dirs.last or dirs.pop
      for dir in dirs
	File.directory?(dir) or print "mkdir -p #{dir}\n"
      end
    end
    def install(file, dir, mode = nil, verbose = false)
      to = catname(file, dir)
      unless FileTest.exist? to and cmp file, to
	print "install#{' -m %#o'%mode if mode} #{file} #{dir}\n"
      end
    end
    def makelink(orig, link, verbose = false)
      unless File.symlink?(link) and File.readlink(link) == orig
	print "ln -sf #{orig} #{link}\n"
      end
    end
  else
    require "ftools"
    def makelink(orig, link, verbose = false)
      if exist? link
	delete link
      end
      symlink orig, link
      print "link #{orig} -> #{link}\n"
    end
  end
end

exeext = CONFIG["EXEEXT"]

ruby_install_name = CONFIG["ruby_install_name"]
rubyw_install_name = CONFIG["rubyw_install_name"]

version = CONFIG["ruby_version"]
bindir = destdir+CONFIG["bindir"]
libdir = destdir+CONFIG["libdir"]
rubylibdir = destdir+CONFIG["rubylibdir"]
archlibdir = destdir+CONFIG["archdir"]
sitelibdir = destdir+CONFIG["sitelibdir"]
sitearchlibdir = destdir+CONFIG["sitearchdir"]
mandir = destdir+CONFIG["mandir"] + "/man1"
dll = CONFIG["LIBRUBY_SO"]
lib = CONFIG["LIBRUBY"]
arc = CONFIG["LIBRUBY_A"]

Installer.makedirs bindir, libdir, rubylibdir, archlibdir, sitelibdir, sitearchlibdir, mandir, true

Installer.install ruby_install_name+exeext, bindir+"/"+ruby_install_name+exeext, 0755, true
if rubyw_install_name and !rubyw_install_name.empty?
  Installer.install rubyw_install_name+exeext, bindir, 0755, true
end
Installer.install dll, bindir, 0755, true unless dll == lib
Installer.install lib, libdir, 0555, true unless lib == arc
Installer.install arc, archlibdir, 0644, true
Installer.install "config.h", archlibdir, 0644, true
Installer.install "rbconfig.rb", archlibdir, 0644, true
if CONFIG["ARCHFILE"]
  for file in CONFIG["ARCHFILE"].split
    Installer.install file, archlibdir, 0644, true
  end
end

if dll == lib and dll != arc
  for link in CONFIG["LIBRUBY_ALIASES"].split
    Installer.makelink(dll, File.join(libdir, link), true)
  end
end

Dir.chdir CONFIG["srcdir"]

Installer.install "sample/irb.rb", "#{bindir}/irb", 0755, true

Dir.glob("lib/*{.rb,help-message}") do |f|
  dir = File.dirname(f).sub!(/\Alib/, rubylibdir) || rubylibdir
  Installer.makedirs dir, true unless File.directory? dir
  Installer.install f, dir, 0644, true
end

for f in Dir["*.h"]
  Installer.install f, archlibdir, 0644, true
end
if RUBY_PLATFORM =~ /mswin32|mingw|bccwin32/
  Installer.makedirs archlibdir + "/win32", true
  Installer.install "win32/win32.h", archlibdir + "/win32", 0644, true
end

Installer.makedirs mandir, true
Installer.install "ruby.1", mandir+"/"+ruby_install_name+".1", 0644, true

# vi:set sw=2:
