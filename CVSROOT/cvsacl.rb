#
# CVS ACL checker module
#
# $devId: cvsacl.rb,v 1.1 2001/01/30 15:39:47 knu Exp $
# $Id$
#

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
	command = fields[0].intern

	case command
	when :host
	  require 'socket'
	  hostname = Socket.gethostbyname(Socket.gethostname)[0].downcase

	  repohosts = fields[1].split(',')

	  unless repohosts.find { |h| h.downcase == hostname }
	    puts "ERROR: You are committing on `#{hostname}'!  Please specify CVSROOT properly and commit on `#{repohosts[0]}'."
	    return false
	  end
	when :group
	  Group.add(fields[1], fields[2].split(','))
	when :deny, :grant
	  if Group.new(fields[1].split(',')).member? user
	    re = Regexp.new("^/?(?:#{fields[2]})(?:/|$)", 'i')

	    if re =~ modulename
	      if command == :deny
		puts "ERROR: Access denied."
		return false
	      end

	      return true
	    end
	  end
	end
      end
    end
  end

  true		# Grant by default
end
