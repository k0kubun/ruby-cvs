#!/usr/bin/env ruby
#
# CVS commit mail script (pre-commit accumulation script)
# This script is run before each commit.
#
# Use only args, not STDIN.
# ARG0 : $CVSROOT
# ARG1 : Absolute path of the module
# ARG2 : CVS user name
# ARG3 : filename of the first file
# ARG4 : filename of the second file
# ..
#
# $Idaemons: /home/cvs/cvsmailer/commitinfo.rb,v 1.2 2001/01/14 09:54:20 knu Exp $
# $Id$

# sanity check: 
require 'socket'
hostname = Socket.gethostbyname(Socket.gethostname)[0]

case hostname
when "helium.ruby-lang.org", "cvs.ruby-lang.org"
  # ok
else
  puts "ERROR: You are committing on `#{hostname}'!  Please specify CVSROOT properly and commit on `cvs.ruby-lang.org'."
  exit 1	# No way!
end

# append a line to a file
def appendline(fn, s)
  File.open(fn, "a+").puts s
end

if ARGV.size < 4
  puts "Usage: #{$0} CVSROOT USER module file file file..."
  exit 1	# No way!
end

cvsroot, cvsuser, modulename, *files = *ARGV

savefile = sprintf("/tmp/commitinfo.%s.%d", cvsuser, Process.getpgrp())

appendline savefile, "#{cvsroot} #{modulename} #{files.join(' ')}"

exit 0		# Let's do it!

# END OF THE SCRIPT
