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

require "singleton"
require "eruby"

module Apache
  class ERuby
    include Singleton

    def handler(r)
      begin
	open(r.filename) do |f|
	  code = @compiler.compile_file(f)
	  eval(code, TOPLEVEL_BINDING)
	  unless ::ERuby.noheader
	    r.content_type = format("text/html; charset=%s", ::ERuby.charset)
	    r.send_http_header
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
