=begin

= apache/ruby.rb

Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

== Overview

Apache::Ruby executes Ruby scripts.

== Example of httpd.conf

  RubyRequire apache/ruby
  <Location /ruby>
  SetHandler ruby-object
  RubyHandler Apache::Ruby.instance
  </Location>

=end

require "singleton"

module Apache
  class Ruby
    include Singleton

    def handler(r)
      if r.finfo.mode == 0
	return Apache::NOT_FOUND
      end
      if r.finfo.directory? or !r.finfo.readable?
	return Apache::FORBIDDEN
      end
      emulate_cgi(r) do
	load(r.filename, true)
      end
      return Apache::OK
    end

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
