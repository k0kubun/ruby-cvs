=begin

= apache/rd2html.rb

Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

=end

require "singleton"
require "rd/rdfmt"
require "rd/rd2html-lib"

module Apache
  class Ruby
    include Singleton

    def handler(r)
      begin
	load(r.filename, true)
	return Apache::OK
      rescue Errno::ENOENT
	return Apache::NOT_FOUND
      rescue Errno::EACCES
	return Apache::FORBIDDEN
      end
    end
  end
end
