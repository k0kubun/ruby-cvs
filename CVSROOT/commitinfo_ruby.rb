#!/usr/bin/env ruby
#
# CVS commit mail script (pre-commit accumulation script)
# This script is run before each commit.
#
# Use only args, not STDIN.
# ARG0 : $CVSROOT
# ARG1 : CVS user name
# ARG2 : absolute path of the module
# ARG3 : file1
# ARG4 : file2
# ..
#
# $Idaemons: /home/cvs/cvsmailer/commitinfo.rb,v 1.4 2001/01/19 17:22:53 knu Exp $
# $devId: commitinfo.rb,v 1.7 2001/01/31 05:18:14 knu Exp $
# $Id$
#

if ARGV.size < 4
  puts "Usage: #{$0} CVSROOT USER module file1 [file2...]"
  exit 1	# No way!
end

$cvsroot, $cvsuser, $modulename, *$cvsfiles = *ARGV

$cvsroot.tr_s!('/', '/')
$modulename.tr_s!('/', '/')

$aclfile = "#{$cvsroot}/CVSROOT/acl"

require "socket"
$hostname = Socket.gethostbyname(Socket.gethostname)[0]

# ACL check:
$:.unshift "#{$cvsroot}/CVSROOT"
require "cvsacl"

if File.exist?($aclfile) && !check_acl($aclfile, $hostname, $cvsuser, $modulename)
  exit 1	# No way!
end

# append a line to a file
def appendline(fn, s)
  File.open(fn, "a+").puts s
end

savefile = sprintf("/tmp/commitinfo.%s.%d", $cvsuser, Process.getpgrp())

appendline savefile, "#{$cvsroot} #{$modulename} #{$cvsfiles.join(' ')}"

exit 0		# Let's do it!

# END OF THE SCRIPT
