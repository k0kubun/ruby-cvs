=begin

= auto-reload.rb

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

autoreload reloads modified libraries automatically.
This library is useful to develop mod_ruby scripts.

=end

module AutoReload
  LIBRARY_MTIMES = {}

  def AutoReload.find_library(lib)
    if lib !~ /\.rb$/
      lib += ".rb"
    end
    for dir in $:
      file = File.expand_path(lib, dir)
      file.untaint
      return file if File.file?(file)
    end
    return nil
  end

  def require(lib)
    unless file = AutoReload.find_library(lib)
      return super(lib)
    end
    mtime = File.mtime(file)
    if mtime == LIBRARY_MTIMES[file]
      return false
    else
      LIBRARY_MTIMES[file] = mtime
      return load(file)
    end
  end
end

include AutoReload		# override Kernel#require
