=begin

= mod_ruby Class Reference Manual

[((<Index|URL:index.en.html>))
|((<RD format|URL:classes.en.rd>))]

* ((<Apache>))
* ((<Apache::Request>))
* ((<Apache::Table>))

== Apache

A module to provide Apache functions.

=== module functions

--- server_version
      Returns the server version string.

--- server_built
      Returns the server built date string.

--- request
      Returns the current ((<Apache::Request>)) object.

--- unescape_url(str)
      Decodes a URL-encoded string.

[((<Back to Index|mod_ruby Class Reference Manual>))]

== Apache::Request

A class to wrap (({request_rec})) data type.

=== Super Class

Object

=== Included Modules

Enumerable

=== Methods

--- hostname
      Returns the hostname, as set by full URI or Host:.

--- unparsed_uri
      Returns the uri without any parsing performed.

--- uri
      Returns the path portion of the URI.

--- filename
      Returns the filename of the script.

--- path_info
      Returns PATH_INFO.

--- request_time
      Returns the time when the request started.

--- request_method
      Rturns "GET", "HEAD", "POST".

--- header_only?
      Returns true if HEAD request.

--- args
      Returns QUERY_ARGS.

--- headers_in
      Returns the ((<Apache::Table>)) object for the request header.

--- read([len])
--- gets([rs])
--- readline([rs])
--- readlines([rs])
--- each([rs]) {|line|...}
--- each_line([rs]) {|line|...}
--- each_byte {|ch|...}
--- getc
--- readchar
--- ungetc(ch)
--- tell
--- seek(offset, [whence])
--- rewind
--- pos
--- pos= n
--- eof
--- eof?
--- binmode
      Receive data from the client.
      These methos work like same methods in IO.

--- status_line= str
      Specifies the status line.

--- status_line
      Returns the specified status line.

--- headers_out
      Returns the ((<Apache::Table>)) object for the response header.

--- content_type= str
      Specifies Content-Type of the response header.

--- content_type
      Returns specified Content-Type.

--- content_encoding= str
      Specifies Content-Encoding of the response header.

--- content_encoding
      Returns specified Content-Languages.

--- content_languages= str
      Specifies Content-Languages of the response header.

--- content_languages
      Returns specified Content-Languages.

--- send_http_header
      Sends the HTTP response header.
      If you call this method twice or much, only sends once.

--- write(str)
--- putc(ch)
--- print(arg...)
--- printf(fmt, arg...)
--- puts(arg...)
--- << obj
      Sends data to the client.
      These methos work like same methods in IO.

--- replace(str)
      Replaces the output buffer with ((|str|)).

--- cancel
      Clears the output buffer.

--- escape_html(str)
      Escapes &"<>.

[((<Back to Index|mod_ruby Class Reference Manual>))]

== Apache::Table

A class to wrap (({table})) data type.

=== Super Class

Object

=== Included Classes

Enumerable

=== Methods

--- clear
      Clears contents of the table.

--- self[name]
--- get(name)
      Returns the value of ((|name|)).

--- self[name]= val
--- set(name, val)
--- setn(name, val)
--- merge(name, val)
--- mergen(name, val)
--- add(name, val)
--- addn(name, val)
      Sets the value of ((|name|)).

--- unset(name)
      Unsets the value of ((|name|)).

--- each {|key,val|...}
--- each_key {|key|...}
--- each_value {|val|...}
      Iterates over each elements.

[((<Back to Index|mod_ruby Class Reference Manual>))]

=end
