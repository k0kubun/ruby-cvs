=begin

= apache/rd2html.rb

Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

== Overview

mod_rd2html converts RD to HTML.

== Requirements

* ruby 1.6.x or later.
* mod_ruby 0.8.0 or later.
* RDtool 0.6.7 or later.

== Example of httpd.conf

  RubyAddPath /usr/lib/apache/1.3/ruby
  RubyRequire apache/ruby
  Alias /ruby-lib-doc/ /usr/lib/ruby/1.6/
  <Location /ruby-lib-doc>
  Options Indexes
  SetHandler ruby-object
  RubyHandler Apache::RD2HTML.instance
  </Location>

You can see the HTML version of ruby library documents at
<URL:http://your.host.name/ruby-lib-doc/>.
If there are no RD documents in a script, mod_rd2html returns
"415 Unsupported Media Type".

=end

require "singleton"
require "rd/rdfmt"
require "rd/rd2html-lib"

module Apache
  class RD2HTML
    include Singleton

    def handler(r)
      begin
	open(r.filename) do |f|
	  tree = RD::RDTree.new(f)
	  visitor = RD::RD2HTMLVisitor.new
	  r.content_type = "text/html"
	  r.send_http_header
	  r.print(visitor.visit(tree))
	  return Apache::OK
	end
      rescue Errno::ENOENT
	return Apache::NOT_FOUND
      rescue Errno::EACCES
	return Apache::FORBIDDEN
      rescue NameError # no =begin ... =end ?
	return Apache::HTTP_UNSUPPORTED_MEDIA_TYPE
      end
    end
  end
end
