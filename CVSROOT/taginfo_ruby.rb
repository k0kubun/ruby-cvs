#!/usr/bin/env ruby
#
# CVS tag info script
# This script is run before each tag/rtag.
#
# Use only args, not STDIN.
# ARG0 : $CVSROOT
# ARG1 : CVS user name
# ARG2 : tagname
# ARG3 : operation ("add" for tag, "mov" for tag -F, and "del" for tag -d)
# ARG4 : absolute path of the module
# ARG5 : file1
# ARG6 : revision1
# ..
#
# $devId: taginfo.rb,v 1.2 2001/01/31 05:18:14 knu Exp $
# $Id$
#

if ARGV.size < 6
  puts "Usage: #{$0} CVSROOT USER tagname operation module file1 rev1 [file2 rev2 ...]"
  exit 1	# No way!
end

$cvsroot, $cvsuser, $tagname, $operation, $modulename, *$cvsfilerevs = *ARGV

$cvsroot.tr_s!('/', '/')
$modulename.tr_s!('/', '/')

$aclfile = "#{$cvsroot}/acl"

require "socket"
$hostname = Socket.gethostbyname(Socket.gethostname)[0]

# acl check:
$:.unshift $cvsroot
require "cvsacl"

if File.exist?($aclfile) && !check_acl($aclfile, $hostname, $cvsuser, $modulename)
  exit 1	# No way!
end

exit 0		# Let's do it!

# END OF THE SCRIPT
