#!/usr/bin/env ruby
#
# CVS commit mail script (loginfo)
#
# ChangeLog:
# 0.1 Created usable version by Kengo Nakajima
# 0.11 Added "_nomail_" command 
# 0.12 subject_prefix, smtp_ipaddr, multiple-email-address. (2000/6/1)
#
# $Idaemons: /home/cvs/cvsmailer/loginfo.rb,v 1.3 2001/01/15 19:42:12 knu Exp $
# $Id$
#

$cvs = "/usr/bin/cvs"


def time2str(tm)
  ret = tm.strftime("%a, #{tm.mday} %b %Y %X ")
  gmt = Time.at(tm.to_i)
  gmt.gmtime
  offset = tm.to_i - Time.local(*gmt.to_a[0,6].reverse).to_i
  ret + sprintf('%+.2d%.2d', *(offset / 60).divmod(60))
end

$datestr = time2str(Time.now)
$ecvsuser = ENV['LOGNAME']
$pgrp = Process.getpgrp()

#
# parse arguments
#

if ARGV.size != 8
  puts "Usage: #{$0} CVSROOT USER 'CVS-LOG-STRING' MAILADDR HELO_DOMAIN\n SMTP_IP SUBJECT_PREFIX X_HEADER_PREFIX"
  exit 1	# No way!
end

puts "Invoking loginfo.rb...."

$cvsroot, $cvsuser, $cvsarg, $mailaddr, $helo_domain, $smtp_ipaddr, $subject_prefix, $x_header_prefix = *ARGV

# Temporary control files
$commitinfosavefile	= sprintf("/tmp/commitinfo.%s.%d", $cvsuser, $pgrp)
$loginfosavefile	= sprintf("/tmp/loginfo.%s.%d", $cvsuser, $pgrp)
$changelogfile		= sprintf("/tmp/loginfo.changelog.%s.%d", $cvsuser, $pgrp)
$addlogfile		= sprintf("/tmp/loginfo.addlog.%s.%d", $cvsuser, $pgrp)
$modlogfile		= sprintf("/tmp/loginfo.modlog.%s.%d", $cvsuser, $pgrp)
$removelogfile		= sprintf("/tmp/loginfo.removelog.%s.%d", $cvsuser, $pgrp)

def cleanup_tmpfiles
  for f in [
      $commitinfosavefile,
      $loginfosavefile,
      $changelogfile,
      $addlogfile,
      $modlogfile,
      $removelogfile ]
    File.unlink(f) rescue true
  end
end

def read_file(fn)
  File.open(fn, "r") do |f|
    f.read
  end
end

def append_line(fn, s)
  File.open(fn, "a+") do |f|
    f.puts s
  end
end

class String
  def indent(n)
    self.gsub(/^/, ' ' * n)
  end
end

def build_header(subject)
  subject = "[#{$subject_prefix}] #{subject}"

  return <<EOF
Return-Path: #{$ecvsuser}@#{$helo_domain}
From: #{$ecvsuser}@#{$helo_domain}
Date: #{$datestr}
Subject: #{subject}
To: #{$mailaddr}
Sender: #{$ecvsuser}@#{$helo_domain}
#{$x_header_prefix}CVS-User: #{$cvsuser}
#{$x_header_prefix}CVS-Root: #{$cvsroot}
#{$x_header_prefix}CVS-Module: #{$modulename}
#{$x_header_prefix}CVS-Branch: #{$branch}
EOF
end

def build_bodyheader
  '%-10s  %s' % [$cvsuser, $datestr] + "\n\n"
end

def build_mail(subject, body)
  build_header(subject) + "\n" + body
end

def send_mail(mail)
  require "net/smtp"
  require "kconv"

  begin
    sm = Net::SMTPSession.new($smtp_ipaddr, 25)
    sm.start($helo_domain)
    sm.sendmail(Kconv.tojis(mail), "#{$ecvsuser}@#{$helo_domain}", $mailaddr.split(/,/))
  rescue
    puts "ERROR: cannot send email using MTA on #{$smtp_ipaddr}"
  end
end


# Save the first argument in a file
# File name is formatted as : "loginfo.%s.%d"

append_line($loginfosavefile, $cvsarg)

commithash = {}
loghash = {}

$isimport = ($cvsarg =~ /Imported sources$/)

if !$isimport
  # compare commitinfo temp files and loginfo temp files.
  # If the two files are logically equal, we know this time is the last one.

  begin
    cfile = File.open($commitinfosavefile, "r")
    lfile = File.open($loginfosavefile, "r")
  rescue => e
    print e.message
    cleanup_tmpfiles
    exit 0
  end

  cfile.each_line { |i|
    i.chomp!
    tk = i.split(/ /)
    croot = tk.shift + "/"      # CVSROOT mod-path file file file...
    cmodpath = tk.shift.sub(/^#{croot}/, "")
    tk.each { |j|
      commithash["#{cmodpath}/#{j}"] = true
    }
  }
  lfile.each_line { |i|
    i.chomp!
    tk = i.split(/ /)
    lmodpath = tk.shift
    tk.each { |j|
      loghash["#{lmodpath}/#{j}"] = true
    }
  }


  # Use "cvs log" and "cvs status" and save output string
  print "loginfo.rb is writing changelog..."

  cvsargtk = $cvsarg.split(/ /)
  $modulename = cvsargtk.shift

  cvsargtk.each { |f|
    revision = ""
    status = `#{$cvs} -Qn status #{f}`
    status.split(/\n/).each { |i|
      if i =~ /Repository revision:\s+(.+)\s+/
	revision = $1
	break
      end
    }
    log = `#{$cvs} -Qn log #{f}`.split(/\n/)
    while true
      line1 = log.shift
      line1.strip!
      if line1 =~ /^revision\s(.+)$/
	if $1 == revision
	  nextline = log.shift
	  nextline.strip!
	  nextline =~ /lines:\s+(.+)\s+(.+)$/
	  append_line($changelogfile, 
		      sprintf("%-9s %-4s %-4s  %s/%s",
			      revision, $1, $2, $modulename, f))
	  break
	end
      end
    end				   
  }

  puts "done"
end

# parse STDIN and sort out messages
print "loginfo.rb is parsing log message..." 

readmode = :init
updatemsg = ""
logtext = ""
statustext = ""
$branch = "HEAD"

STDIN.each_line do |l|
  case l
  when /^Update of (.*)/
    updatemsg = $1.strip
    next
  when /^Modified Files:/
    readmode = :modify
    next
  when /^Added Files:/
    readmode = :add
    next
  when /^Removed Files:/
    readmode = :remove
    next
  when /^Log Message:/
    readmode = :log
    next
  when /^Status:/
    readmode = :status
    next
  when nil
    break
  end

  case readmode
  when :modify
    if l =~ /^\s+Tag:\s+(\S+)$/
      $branch = $1
    else
      append_line($modlogfile, l.strip)
    end
  when :add
    append_line($addlogfile, l.strip)
  when :remove
    append_line($removelogfile, l.strip)
  when :log
    logtext += l
  when :status
    statustext += l
  end
end

puts "done"

# We need more modules...
print "loginfo.rb is testing modules..."
commithash.each_key { |k|
  if loghash[k] == nil   
    puts "log info for `#{k}' is not ready" 
    exit 0
  end
}
puts "done"


#
# we can send a mail.
#

print "loginfo.rb is composing a mail..."

# Use the first valuable line as the subject of the mail.
subject = "#{$modulename}: "
logtext.each_line { |i|
  i.strip!
  if i != ""
    subject += i
    break
  end
}

branchinfo = if $branch == 'HEAD'
	       ""
	     else
	       "        (Branch: #{$branch})"
	     end

logtext.chomp!

body =
  build_bodyheader +
  ("Module:\n" +
   updatemsg.indent(2) +
   "\n").indent(2)

if $isimport
  body +=
    ("Log:\n" +
     logtext.chomp.indent(2) + "\n" +
     "\n" +
     "Imported files:#{branchinfo}\n" +
     statustext.indent(2)).indent(2)
else
  body +=
    (("Modified files:#{branchinfo}\n" +
     read_file($modlogfile).indent(2) rescue "") +
     ("Added files:#{branchinfo}\n" +
      read_file($addlogfile).indent(2) rescue "") +
     ("Removed files:#{branchinfo}\n" +
      read_file($removelogfile).indent(2) rescue "") +
     "Log:\n" +
     logtext.chomp.indent(2) + "\n" +
     "\n" +
     ("Revision  Changes    Path\n" +
      read_file($changelogfile).chomp rescue "")).indent(2)
end

mail = build_mail(subject, body)

puts "done"

# detect _nomail_
if logtext =~ /_nomail_/
  puts "loginfo.rb is not posting email."
  puts "The composed mail is as follows:"
  puts mail
else
  print "loginfo.rb is posting email to #{$mailaddr} ..."
  send_mail(mail)
  puts "done"
end

# remove all temp files

print "loginfo.rb is deleting tmp files..."
cleanup_tmpfiles

puts "done"

exit 0
