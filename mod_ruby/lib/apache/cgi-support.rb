=begin

= apache/cgi-support.rb

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

Apache::CGISupport provides CGI emulation.

=end

module Apache
  module CGISupport
    private

    def emulate_cgi(r)
      r.setup_cgi_env
      Apache.chdir_file(r.filename)
      stdin, stdout, defout = $stdin, $stdout, $>
      $stdin = $stdout = $> = r
      begin
	yield
      ensure
	$stdin, $stdout, $> = stdin, stdout, defout
      end
    end
  end
end
