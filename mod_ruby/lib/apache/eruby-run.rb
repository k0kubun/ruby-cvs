=begin

= apache/eruby-run.rb

Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>

This file is part of mod_ruby.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA.

== Overview

Apache::ERubyRun handles eRuby files.

== Example of httpd.conf

  RubyRequire apache/eruby-run
  <Location /eruby>
  SetHandler ruby-object
  RubyHandler Apache::ERubyRun.instance
  </Location>

=end

require "singleton"
require "apache/cgi-support"

require "eruby"

module ERuby
  @@cgi = nil

  def self.cgi
    return @@cgi
  end

  def self.cgi=(cgi)
    @@cgi = cgi
  end
end

module Apache
  class ERubyRun
    include Singleton
    include CGISupport

    def handler(r)
      if r.finfo.mode == 0
	return NOT_FOUND
      end
      open(r.filename) do |f|
	ERuby.noheader = false
	ERuby.charset = ERuby.default_charset
	ERuby.cgi = nil
	@compiler.sourcefile = r.filename
	code = @compiler.compile_file(f)
	emulate_cgi(r) do
	  eval(code, TOPLEVEL_BINDING, r.filename)
	  unless ERuby.noheader
	    if cgi = ERuby.cgi
	      cgi.header("charset" => ERuby.charset)
	    else
	      r.content_type = format("text/html; charset=%s", ERuby.charset)
	      r.send_http_header
	    end
	  end
	end
      end
      return OK
    end

    private

    def initialize
      @compiler = ERuby::Compiler.new
    end
  end
end
