=begin

= apache/eruby-run.rb

Copyright (C) 2001  Shugo Maeda <shugo@modruby.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

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
require "tempfile"
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

    def handler(r)
      if r.method_number == M_OPTIONS
	r.allowed |= (1 << M_GET)
	r.allowed |= (1 << M_POST)
	return DECLINED
      end
      if r.finfo.mode == 0
	return NOT_FOUND
      end

      filename = r.filename.dup
      filename.untaint
      code = compile(filename)
      prerun(r)
      run(code, filename)
      postrun(r)

      return OK
    end

    private

    def initialize
      @compiler = ERuby::Compiler.new
    end

    def compile(filename)
      open(filename) do |f|
	@compiler.sourcefile = filename
	return @compiler.compile_file(f)
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
