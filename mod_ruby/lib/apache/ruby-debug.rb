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
require "cgi"

module Apache
  class RubyDebug
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
      rescue SystemExit
	raise($!)
      rescue Exception
        r.content_type = "text/html"
	r.send_http_header

        $>.replace('')

        bt     = $!.backtrace.collect{|x| CGI::escapeHTML(x)}
        msg    = CGI::escapeHTML($!.to_s)
        code   = File::open(filename){|fd| fd.read}

        lineno = bt.shift.scan(/:(\d+)/).shift.shift.to_i
        err = sprintf("%s:%d: %s<br>\n", filename, lineno, msg)
        bt.each do |stack|
          err << sprintf("<div class=\"backtrace\">%s<br></div>\n", stack)
        end

        print_error(r, code, err)
      end
      return OK
    end

    def print_error(r, code, err)
      r.print <<HTML
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN">
<html>
  <head>
    <title>mod_ruby</title>
    <style type="text/css">
      <!--
        body { background-color: #ffffff }
        table { width: 100%; padding: 5pt; border-style: none }
        th { color: #6666ff; background-color: #b0d0d0; text-align: left }
        td { color: #336666; background-color: #d0ffff }
        strong { color: #ff0000; font-weight: bold }
        div.backtrace { text-indent: 15% }
        #version { color: #ff9900 }
      -->
    </style>
  </head>
  <body>
    <table summary="mod_ruby debug information">
      <tr><th id="error">
        ERROR
      </th></tr>
      <tr><td headers="error">
        #{err}
      </td></tr>
      <tr><th id="code">
        CODE
      </th></tr>
      <tr><td headers="code">
        <pre><code>#{CGI::escapeHTML(code)}</code></pre>
      </td></tr>
    </table>
  </body>
</html>
HTML
    end
  end
end
