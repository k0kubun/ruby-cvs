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
# $Idaemons: /home/cvs/cvsmailer/commitinfo.rb,v 1.4 2001/01/19 17:22:53 knu Exp $
# $Id$
#

if ARGV.size < 4
  puts "Usage: #{$0} CVSROOT USER module file file file..."
  exit 1	# No way!
end

$cvsroot, $cvsuser, $modulename, *$cvsfiles = *ARGV

$cvsroot.tr_s!('/', '/')
$modulename.tr_s!('/', '/')

$aclfile = "#{$cvsroot}/acl"

# sanity check: 
require 'socket'
hostname = Socket.gethostbyname(Socket.gethostname)[0]

case hostname.downcase
when "helium.ruby-lang.org", "cvs.ruby-lang.org"
  # ok
else
  puts "ERROR: You are committing on `#{hostname}'!  Please specify CVSROOT properly and commit on `cvs.ruby-lang.org'."
  exit 1	# No way!
end

# acl check:
class Group
  @@group_table = {}

  def Group::add(gid, obj)
    @@group_table[gid] = if obj.is_a?(Group) then obj else Group.new(obj) end
    self
  end

  def Group::group(gid)
    if @@group_table.key? gid
      @@group_table[gid]
    else
      nil
    end
  end

  def initialize(list)
    @members = []
    @all = false

    return if list == nil

    list.each do |e|
      if e =~ /^@(.*)/
	gid = $1

	g = Group.group(gid)

	if g == nil
	  puts "No such group defined: '#{gid}'"
	  next
	end

	if g.all?
	  all!
	  break
	end

	@members |= g.members
      else
	@members |= [e]
      end
    end
  end

  def all?
    @all
  end

  def all!
    @all = true
    @member = nil
    self
  end

  def member?(u)
    @all || @members.member?(u)
  end

  def members
    @members
  end

  # Register a special group named 'all'
  add('all', new(nil).all!)
end

def check_acl(aclfile, user, modulename)
  File.open(aclfile) do |f|
    f.each_line do |line|
      line.strip!
      case line
      when /^#/, /^$/
	next
      else
	fields = line.split(':')
	command = fields[0]

	case command
	when 'group'
	  Group.add(fields[1], fields[2].split(','))
	when 'deny', 'grant'
	  if Group.new(fields[1].split(',')).member? user
	    re = Regexp.new("^/?(?:#{fields[2]})(?:/|$)")

	    if re =~ modulename
	      return (command == 'grant')
	    end
	  end
	end
      end
    end
  end

  true		# Grant by default
end

if File.exist?($aclfile) && !check_acl($aclfile, $cvsuser, $modulename)
  puts "ERROR: You are not granted to commit on #{$modulename}'!"
  exit 2	# No way!
end

# append a line to a file
def appendline(fn, s)
  File.open(fn, "a+").puts s
end

savefile = sprintf("/tmp/commitinfo.%s.%d", $cvsuser, Process.getpgrp())

appendline savefile, "#{$cvsroot} #{$modulename} #{$cvsfiles.join(' ')}"

exit 0		# Let's do it!

# END OF THE SCRIPT
