=begin

= apache/erb-run.rb

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

Apache::ERbRun handles eRuby files by ERb.

== Example of httpd.conf

  RubyRequire apache/erb-run
  <Location /erb>
  SetHandler ruby-object
  RubyHandler Apache::ERbRun.instance
  </Location>

=end

require "singleton"
require "tempfile"
require "erb/compile"

# eruby emulation
if !defined?(ERuby)
  module ERuby
    @@noheader = false
    @@charset = @@default_charset = "iso-8859-1"
    @@cgi = nil
  
    def ERuby.noheader
      return @@noheader
    end

    def ERuby.noheader=(val)
      @@noheader = val
      return val
    end
  
    def ERuby.charset
      return @@charset
    end

    def ERuby.charset=(val)
      @@charset = val
      return val
    end
  
    def ERuby.default_charset
      return @@default_charset
    end

    def self.cgi
      return @@cgi
    end

    def self.cgi=(cgi)
      @@cgi = cgi
    end
  end
end

module Apache
  class ERbRun
    include Singleton

    def handler(r)
      if r.finfo.mode == 0
	return NOT_FOUND
      end

      code = compile(r.filename)
      prerun(r)
      run(code, r.filename)
      postrun(r)

      return OK
    end

    private

    def initialize
      @compiler = ERbCompiler.new
      @compiler.put_cmd = 'print'
    end

    def compile(filename)
      open(filename) do |f|
	return @compiler.compile(f.read)
      end
    end

    def prerun(r)
      ERuby.noheader = false
      ERuby.charset = ERuby.default_charset
      ERuby.cgi = nil
      r.setup_cgi_env
      Apache.chdir_file(r.filename)
    end

    def run(code, filename)
      file = Tempfile.new(File.basename(filename) + ".")
      begin
	file.print(code)
	file.close
	load(file.path, true)
      ensure
	file.close(true)
      end
    end

    def postrun(r)
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
end
