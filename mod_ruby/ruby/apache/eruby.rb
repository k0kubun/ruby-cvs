=begin

= apache/eruby.rb

Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

== Overview

Apache::ERuby handles eRuby files.

== Example of httpd.conf

  RubyRequire apache/eruby
  <Location /eruby>
  SetHandler ruby-object
  RubyHandler Apache::ERuby.instance
  </Location>

=end

require "cgi/session"

require "eruby"
require "apache/ruby"

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
  class ERuby < Apache::Ruby
    def handler(r)
      begin
	open(r.filename) do |f|
	  ::ERuby.noheader = false
	  ::ERuby.charset = ::ERuby.default_charset
	  ::ERuby.cgi = nil
	  code = @compiler.compile_file(f)
	  emulate_cgi(r) do
	    eval(code, TOPLEVEL_BINDING)
	    unless ::ERuby.noheader
	      if cgi = ::ERuby.cgi
		cgi.header("charset" => ::ERuby.charset)
	      else
		r.content_type = format("text/html; charset=%s", ::ERuby.charset)
		r.send_http_header
	      end
	    end
	  end
	end
	return Apache::OK
      rescue Errno::ENOENT
	return Apache::NOT_FOUND
      rescue Errno::EACCES
	return Apache::FORBIDDEN
      end
    end

    private

    def initialize
      @compiler = ::ERuby::Compiler.new
    end
  end
end
