=begin

= apache/ruby-run.rb

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

Apache::RubyRun executes Ruby scripts.

== Example of httpd.conf

  RubyRequire apache/ruby-run
  <Location /ruby>
  SetHandler ruby-object
  RubyHandler Apache::RubyRun.instance
  </Location>

=end

require "singleton"

module Apache
  class RubyRun
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
      Apache.chdir_file(r.filename)
      load(r.filename, true)
      return OK
    end
  end
end
