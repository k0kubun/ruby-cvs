=begin

= apache/ruby-debug.rb

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

Apache::RubyDebug executes Ruby scripts, and sends information to clients
on error for debug.

== Example of httpd.conf

  RubyRequire apache/ruby-debug
  <Location /ruby-debug>
  SetHandler ruby-object
  RubyHandler Apache::RubyDebug.instance
  </Location>

=end

require "singleton"

module Apache
  class RubyDebug
    include Singleton

    def handler(r)
      if r.finfo.mode == 0
	return NOT_FOUND
      end
      if r.allow_options & OPT_EXECCGI == 0
	r.log_reason("Options ExecCGI is off in this directory", r.filename)
	return FORBIDDEN
      end
      unless r.finfo.executable?
	r.log_reason("file permissions deny server execution", r.filename)
	return FORBIDDEN
      end
      r.setup_cgi_env
      filename = r.filename.dup
      filename.untaint
      Apache.chdir_file(filename)
      begin
	load(filename, true)
      rescue Exception
	r.content_type = "text/plain"
	r.send_http_header
	r.printf("error: %s\n\n", $!)
	r.print("backtrace:\n")
	for i in $!.backtrace
	  r.printf("\t%s\n", i)
	end
      end
      return OK
    end
  end
end
