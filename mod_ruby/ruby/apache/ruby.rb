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
      st = r.finfo
      if st.mode.zero?
	return Apache::NOT_FOUND
      end
      if st.directory? or !st.readable?
	return Apache::FORBIDDEN
      end
      load(r.filename, true)
      return Apache::OK
    end
  end
end
