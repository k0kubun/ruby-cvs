require "tempfile"
require "rbconfig.rb"

require "rubicon_tests"

#
# Simple wrapper for RubyUnit, primarily designed to capture
# statsistics and report them at the end.
#

# -------------------------------------------------------------
#
# Operating system classification. We use classes for this, as 
# we get lots of flexibility with comparisons.
#
# Use with
#
#   Unix.or_varient do ... end        # operating system is some Unix variant
#
#   Linux.only do .... end            # operating system is Linux
#
#   MsWin32.dont do .... end          # don't run under MsWin32

class OS
  def OS.or_variant
    yield if $os <= self
  end

  def OS.only
    yield if $os == self
  end

  def OS.dont
    yield unless $os <= self
  end
end

class Unix    < OS;      end
class Linux   < Unix;    end
class BSD     < Unix;    end
class FreeBSD < BSD;     end
class Solaris < Unix;    end

class Windows < OS;      end
class Cygwin  < Windows; end

class WindowsNative < Windows; end
class MsWin32 < WindowsNative; end
class MinGW   < WindowsNative; end

$os = case RUBY_PLATFORM
      when /linux/   then  Linux
      when /bsd/     then BSD
      when /solaris/ then Solaris
      when /cygwin/  then Cygwin
      when /mswin32/ then MsWin32
      when /mingw32/ then MinGW
      else OS
      end


#
# Find the name of the interpreter.
# 

$interpreter = File.join(Config::CONFIG["bindir"], 
			 Config::CONFIG["RUBY_INSTALL_NAME"])

MsWin32.or_variant { $interpreter.tr! '/', '\\' }


######################################################################
#
# This is tacky, but... We need to be able tofind the executable
# files in the util subdirectory. However, we can be initiated by
# running a file in either the tpo-level rubicon directory or in
# one of its test subdirectories (such as language). We therefore
# need tpo hunt around for the util directory

run_dir = File.dirname($0)

for relative_path in [ ".", ".." ]
  util = File.join(run_dir, relative_path, "util")

  if File.exist?(util) and File.directory?(util)
    UTIL = util
    break
  end
end

raise "Cannot find 'util' directory" unless defined?(UTIL)

CHECKSTAT = File.join(UTIL, "checkstat")
TEST_TOUCH = File.join(UTIL, "test_touch")


WindowsNative.or_variant do
  CHECKSTAT << ".exe"
  TEST_TOUCH << ".exe"
end

for file in [CHECKSTAT, TEST_TOUCH]
  raise "Cannot find #{file}" unless File.exist?(file)
end


#
# Classification routines. We use these so that the code can
# test for operating systems, ruby versions, and other features
# without being platform specific
#

# -------------------------------------------------------
# Class to manipulate Ruby version numbers. We use this to 
# insulate ourselves from changes in version number format.
# Independent of the internal representation, we always allow 
# comparison against a string.
#
# Use in the code with stuff like:
#
#    if $rubyVersion <= "1.6.2" 
#       asert(...)
#    end
#

class VersionNumber
  include Comparable
  
  def initialize(version)
    @version = version
  end
  
  def <=>(other)
    @version <=> other
  end
end

$rubyVersion = VersionNumber.new(VERSION)






