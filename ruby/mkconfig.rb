#!./miniruby

require File.dirname($0)+"/lib/ftools"

rbconfig_rb = ARGV[0] || 'rbconfig.rb'
File.makedirs(File.dirname(rbconfig_rb), true)

version = VERSION
config = open(rbconfig_rb, "w")
$stdout.reopen(config)

fast = {'prefix'=>TRUE, 'INSTALL'=>TRUE, 'EXEEXT'=>TRUE}
print %[
module Config

  VERSION == "#{version}" or
    raise "ruby lib version (#{version}) doesn't match executable version (\#{VERSION})"

# This file was created by configrb when ruby was built. Any changes
# made to this file will be lost the next time ruby is built.
]

print "  DESTDIR = '' if not defined? DESTDIR\n  CONFIG = {}\n"
v_fast = []
v_others = []
has_version = false
File.foreach "config.status" do |$_|
  next if /^#/
  if /^s%@program_transform_name@%s,(.*)%g$/
    ptn = $1.sub(/\$\$/, '$').split(/,/)	#'
    v_fast << "  CONFIG[\"ruby_install_name\"] = \"" + "ruby".sub(ptn[0],ptn[1]) + "\"\n"
  elsif /^s%@(\w+)@%(.*)%g/
    name = $1
    val = $2 || ""
    next if name =~ /^(INSTALL|DEFS|configure_input|srcdir|top_srcdir)$/
    v = "  CONFIG[\"" + name + "\"] = " +
      val.sub(/^\s*(.*)\s*$/, '"\1"').gsub(/\$\{?([^(){}]+)\}?/) {
      "\#{CONFIG[\\\"#{$1}\\\"]}"
    } + "\n"
    if fast[name]
      v_fast << v
    else
      v_others << v
    end
    has_version = true if name == "MAJOR"
    if /DEFS/
      val.split(/\s*-D/).each do |i|
	if i =~ /(.*)=(\\")?([^\\]*)(\\")?/
	  key, val = $1, $3
	  if val == '1'
	    val = "TRUE"
	  else
	    val.sub! /^\s*(.*)\s*$/, '"\1"'
	  end
	  print "  CONFIG[\"#{key}\"] = #{val}\n"
	end
      end
    end
  elsif /^ac_given_srcdir=(.*)/
    v_fast << "  CONFIG[\"srcdir\"] = \"" + File.expand_path($1) + "\"\n"
  elsif /^ac_given_INSTALL=(.*)/
    v_fast << "  CONFIG[\"INSTALL\"] = " + $1 + "\n"
  end
#  break if /^CEOF/
end

if not has_version
  VERSION.scan(/(\d+)\.(\d+)\.(\d+)/) {
    print "  CONFIG[\"MAJOR\"] = \"" + $1 + "\"\n"
    print "  CONFIG[\"MINOR\"] = \"" + $2 + "\"\n"
    print "  CONFIG[\"TEENY\"] = \"" + $3 + "\"\n"
  }
end

v_fast.collect! do |x|
  if /"prefix"/ === x
    x.sub(/= /, '= DESTDIR + ')
  else
    x
  end
end

print v_fast, v_others
print <<EOS
  CONFIG["compile_dir"] = "#{Dir.pwd}"
  MAKEFILE_CONFIG = {}
  CONFIG.each{|k,v| MAKEFILE_CONFIG[k] = v.dup}
  def Config::expand(val)
    val.gsub!(/\\$\\(([^()]+)\\)/) do |var|
      key = $1
      if CONFIG.key? key
        "\#{Config::expand(CONFIG[\\\"\#{key}\\\"])}"
      else
	var
      end
    end
    val
  end
  CONFIG.each_value do |val|
    Config::expand(val)
  end
end
EOS
config.close

# vi:set sw=2:
