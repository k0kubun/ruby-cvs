=begin

= apache/rd2html.rb

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

Apache::RD2HTML converts RD to HTML.

== Requirements

* RDtool 0.6.7 or later.

== Example of httpd.conf

  RubyRequire apache/rd2html
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