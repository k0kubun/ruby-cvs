=begin

= net/http.rb

Copyright (c) 1999-2002 Yukihiro Matsumoto

written & maintained by Minero Aoki <aamine@loveruby.net>
This file is derived from "http-access.rb".

This program is free software. You can re-distribute and/or
modify this program under the same terms as Ruby itself,
Ruby Distribute License or GNU General Public License.

NOTE: You can find Japanese version of this document in
the doc/net directory of the standard ruby interpreter package.

$Id$

== What is this module?

This module provide your program the functions to access WWW
documents via HTTP, Hyper Text Transfer Protocol version 1.1.
For details of HTTP, refer [RFC2616]
((<URL:http://www.ietf.org/rfc/rfc2616.txt>)).

== Examples

=== Getting Document From Server

    require 'net/http'
    Net::HTTP.start( 'some.www.server', 80 ) {|http|
        response = http.get('/index.html')
        puts response.body
    }

(shorter version)

    require 'net/http'
    Net::HTTP.get_print 'some.www.server', '/index.html'
    # or
    Net::HTTP.get_print URI.parse('http://www.example.com/index.html')

=== Posting Form Data

    require 'net/http'
    Net::HTTP.start( 'some.www.server', 80 ) {|http|
        response = http.post('/cgi-bin/any.rhtml',
                             'querytype=subject&target=ruby')
    }

=== Accessing via Proxy

Net::HTTP.Proxy() creates http proxy class. It has same
methods of Net::HTTP but its instances always connect to
proxy, instead of given host.

    require 'net/http'

    $proxy_addr = 'your.proxy.addr'
    $proxy_port = 8080
            :
    Net::HTTP::Proxy($proxy_addr, $proxy_port).start('some.www.server') {|http|
        # always connect to your.proxy.addr:8080
            :
    }

Since Net::HTTP.Proxy() returns Net::HTTP itself when $proxy_addr is nil,
there's no need to change code if there's proxy or not.

=== Following Redirection

    require 'net/http'

    def read_uri( uri_str )
      response = Net::HTTP.get_response(URI.parse(uri_str))
      case response
      when Net::HTTPSuccess     then response
      when Net::HTTPRedirection then read_uri(response['location'])
      else
        response.error!
      end
    end

    print read_uri('http://www.ruby-lang.org')

Net::HTTPSuccess and Net::HTTPRedirection is a HTTPResponse class.
All HTTPResponse objects belong to its own response class which
indicate HTTP result status. For details of response classes,
see section "HTTP Response Classes".

=== Basic Authentication

    require 'net/http'

    req = Net::HTTP::Get.new('/need-auth.cgi')
    req.basic_auth 'account', 'password'
    Net::HTTP.start( 'auth.some.domain' ) {|http|
        response = http.request(req)
        print response.body
    }

=== HTTP Response Classes

Followings are sub classes of Net::HTTPResponse. All classes are
defined under the Net module. Indentation indicates inheritance.

  xxx        HTTPResponse

    1xx        HTTPInformation
      100        HTTPContinue    
      101        HTTPSwitchProtocol

    2xx        HTTPSuccess
      200        HTTPOK
      201        HTTPCreated
      202        HTTPAccepted
      203        HTTPNonAuthoritativeInformation
      204        HTTPNoContent
      205        HTTPResetContent
      206        HTTPPartialContent

    3xx        HTTPRedirection
      300        HTTPMultipleChoice
      301        HTTPMovedPermanently
      302        HTTPFound
      303        HTTPSeeOther
      304        HTTPNotModified
      305        HTTPUseProxy
      307        HTTPTemporaryRedirect

    4xx        HTTPClientError
      400        HTTPBadRequest
      401        HTTPUnauthorized
      402        HTTPPaymentRequired
      403        HTTPForbidden
      404        HTTPNotFound
      405        HTTPMethodNotAllowed
      406        HTTPNotAcceptable
      407        HTTPProxyAuthenticationRequired
      408        HTTPRequestTimeOut
      409        HTTPConflict
      410        HTTPGone
      411        HTTPLengthRequired
      412        HTTPPreconditionFailed
      413        HTTPRequestEntityTooLarge
      414        HTTPRequestURITooLong
      415        HTTPUnsupportedMediaType
      416        HTTPRequestedRangeNotSatisfiable
      417        HTTPExpectationFailed

    5xx        HTTPServerError
      500        HTTPInternalServerError
      501        HTTPNotImplemented
      502        HTTPBadGateway
      503        HTTPServiceUnavailable
      504        HTTPGatewayTimeOut
      505        HTTPVersionNotSupported

    xxx        HTTPUnknownResponse

== Switching Net::HTTP versions

You can use net/http.rb 1.1 features (bundled with Ruby 1.6)
by calling HTTP.version_1_1. Calling Net::HTTP.version_1_2
allows you to use 1.2 features again.

    # example
    Net::HTTP.start {|http1| ...(http1 has 1.2 features)... }

    Net::HTTP.version_1_1
    Net::HTTP.start {|http2| ...(http2 has 1.1 features)... }

    Net::HTTP.version_1_2
    Net::HTTP.start {|http3| ...(http3 has 1.2 features)... }

This function is not thread-safe.

== class Net::HTTP

=== Class Methods

: new( address, port = 80, proxy_addr = nil, proxy_port = nil )
    creates a new Net::HTTP object.
    If proxy_addr is given, creates an Net::HTTP object with proxy support.

: start( address, port = 80, proxy_addr = nil, proxy_port = nil )
    creates a new Net::HTTP object and returns it
    with opening HTTP session. 

: start( address, port = 80, proxy_addr = nil, proxy_port = nil ) {|http| .... }
    creates a new Net::HTTP object and gives it to the block.
    HTTP session is kept to open while the block is exected.

    This method returns the return value of the block.

: get_print( uri )
: get_print( address, path, port = 80 )
    gets entity body from the target and output it to stdout.

        Net::HTTP.get_print URI.parse('http://www.example.com')

: get( uri )
: get( address, path, port = 80 )
    send GET request to the target and get a response.
    This method returns a String.

        print Net::HTTP.get(URI.parse('http://www.example.com'))

: get_response( uri )
: get_response( address, path, port = 80 )
    send GET request to the target and get a response.
    This method returns a Net::HTTPResponse object.

        res = Net::HTTP.get_response(URI.parse('http://www.example.com'))
        print res.body

: Proxy( address, port = 80 )
    creates a HTTP proxy class.
    Arguments are address/port of proxy host.
    You can replace HTTP class with created proxy class.

    If ADDRESS is nil, this method returns self (Net::HTTP).

        # example
        proxy_class = Net::HTTP::Proxy( 'proxy.foo.org', 8080 )
                        :
        proxy_class.start( 'www.ruby-lang.org' ) {|http|
            # connecting proxy.foo.org:8080
                        :
        }

: proxy_class?
    If self is HTTP, false.
    If self is a class which was created by HTTP::Proxy(), true.

: port
    default HTTP port (80).

=== Instance Methods

: start
: start {|http| .... }
    opens HTTP session.

    When this method is called with block, gives a HTTP object
    to the block and closes the HTTP session after block call finished.

: started?
    returns true if HTTP session is started.

: address
    the address to connect

: port
    the port number to connect

: open_timeout
: open_timeout=(n)
    seconds to wait until connection is opened.
    If HTTP object cannot open a conection in this seconds,
    it raises TimeoutError exception.

: read_timeout
: read_timeout=(n)
    seconds to wait until reading one block (by one read(1) call).
    If HTTP object cannot open a conection in this seconds,
    it raises TimeoutError exception.

: finish
    finishes HTTP session.
    If HTTP session had not started, raises an IOError.

: proxy?
    true if self is a HTTP proxy class

: proxy_address
    address of proxy host. If self does not use a proxy, nil.

: proxy_port
    port number of proxy host. If self does not use a proxy, nil.

: get( path, header = nil )
: get( path, header = nil ) {|str| .... }
    gets data from PATH on the connecting host.
    HEADER must be a Hash like { 'Accept' => '*/*', ... }.

    In version 1.1, this method returns a pair of objects,
    a Net::HTTPResponse object and entity body string.
    In version 1.2, this method returns a Net::HTTPResponse
    object.

    If called with block, gives entity body string to the block
    little by little.

    In version 1.1, this method might raises exception for also
    3xx (redirect). On the case you can get a HTTPResponse object
    by "anException.response".
    In version 1.2, this method never raises exception.

        # version 1.1 (bundled with Ruby 1.6)
        response, body = http.get( '/index.html' )

        # version 1.2 (bundled with Ruby 1.7 or later)
        response = http.get( '/index.html' )

        # compatible in both version
        response , = http.get( '/index.html' )
        response.body
        
        # using block
        File.open( 'save.txt', 'w' ) {|f|
            http.get( '/~foo/', nil ) do |str|
              f.write str
            end
        }

: head( path, header = nil )
    gets only header from PATH on the connecting host.
    HEADER is a Hash like { 'Accept' => '*/*', ... }.

    This method returns a Net::HTTPResponse object.

    In version 1.1, this method might raises exception for also
    3xx (redirect). On the case you can get a HTTPResponse object
    by "anException.response".
    In version 1.2, this method never raises exception.

        response = nil
        Net::HTTP.start( 'some.www.server', 80 ) {|http|
            response = http.head( '/index.html' )
        }
        p response['content-type']

: post( path, data, header = nil )
: post( path, data, header = nil ) {|str| .... }
    posts DATA (must be String) to PATH. HEADER must be a Hash
    like { 'Accept' => '*/*', ... }.

    In version 1.1, this method returns a pair of objects, a
    Net::HTTPResponse object and an entity body string.
    In version 1.2, this method returns a Net::HTTPReponse object.

    If called with block, gives a part of entity body string.

    In version 1.1, this method might raises exception for also
    3xx (redirect). On the case you can get a HTTPResponse object
    by "anException.response".
    In version 1.2, this method never raises exception.

        # version 1.1
        response, body = http.post( '/cgi-bin/search.rb', 'query=subject&target=ruby' )

        # version 1.2
        response = http.post( '/cgi-bin/search.rb', 'query=subject&target=ruby' )

        # compatible in both version
        response , = http.post( '/cgi-bin/search.rb', 'query=subject&target=ruby' )

        # using block
        File.open( 'save.html', 'w' ) {|f|
            http.post( '/cgi-bin/search.rb',
                       'query=subject&target=ruby' ) do |str|
              f.write str
            end
        }

: request_get( path, header = nil )
: request_get( path, header = nil ) {|response| .... }
    gets entity from PATH. This method returns a HTTPResponse object.

    When called with block, keep connection while block is executed
    and gives a HTTPResponse object to the block.

    This method never raises Net::* exceptions.

        # example
        response = http.request_get( '/index.html' )
        p response['content-type']
        puts response.body          # body is already read

        # using block
        http.request_get( '/index.html' ) {|response|
            p response['content-type']
            response.read_body do |str|   # read body now
              print str
            end
        }

: request_post( path, data, header = nil )
: request_post( path, data, header = nil ) {|response| .... }
    posts data to PATH. This method returns a HTTPResponse object.

    When called with block, gives a HTTPResponse object to the block
    before reading entity body, with keeping connection.

    This method never raises Net::* exceptions.

        # example
        response = http.post2( '/cgi-bin/nice.rb', 'datadatadata...' )
        p response.status
        puts response.body          # body is already read

        # using block
        http.post2( '/cgi-bin/nice.rb', 'datadatadata...' ) {|response|
            p response.status
            p response['content-type']
            response.read_body do |str|   # read body now
              print str
            end
        }

: request( request [, data] )
: request( request [, data] ) {|response| .... }
    sends a HTTPRequest object REQUEST to the HTTP server.
    This method also writes DATA string if REQUEST is a post/put request.
    Giving DATA for get/head request causes ArgumentError.

    If called with block, this method passes a HTTPResponse object to
    the block, without reading entity body.

    This method never raises Net::* exceptions.

== class Net::HTTPRequest

HTTP request class. This class wraps request header and entity path.
You MUST use its subclass, Net::HTTP::Get, Post, Head.

=== Class Methods

: new
    creats HTTP request object.

=== Instance Methods

: self[ key ]
    returns the header field corresponding to the case-insensitive key.
    For example, a key of "Content-Type" might return "text/html"

: self[ key ] = val
    sets the header field corresponding to the case-insensitive key.

: each {|name, val| .... }
    iterates for each field name and value pair.

: basic_auth( account, password )
    set Authorization: header for basic auth.

: range
    returns a Range object which represents Range: header field.

: range = r
: set_range( i, len )
    set Range: header from Range (arg r) or beginning index and
    length from it (arg i&len).

: content_length
    returns a Integer object which represents Content-Length: header field.

: content_range
    returns a Range object which represents Content-Range: header field.

== class Net::HTTPResponse

HTTP response class. This class wraps response header and entity.
All arguments named KEY is case-insensitive.

=== Instance Methods

: self[ key ]
    returns the header field corresponding to the case-insensitive key.
    For example, a key of "Content-Type" might return "text/html".
    A key of "Content-Length" might do "2045".

    More than one fields which has same names are joined with ','.

: self[ key ] = val
    sets the header field corresponding to the case-insensitive key.

: fetch( key [,default] )

    returns the header field corresponding to the case-insensitive key.
    returns the default value if there's no header field named key.

: key?( key )
    true if key exists.
    KEY is case insensitive.

: each {|name,value| .... }
    iterates for each field name and value pair.

: canonical_each {|name,value| .... }
    iterates for each "canonical" field name and value pair.

: code
    HTTP result code string. For example, '302'.

: message
    HTTP result message. For example, 'Not Found'.

: read_body( dest = '' )
    gets entity body and write it into DEST using "<<" method.
    If this method is called twice or more, nothing will be done
    and returns first DEST.

: read_body {|str| .... }
    gets entity body little by little and pass it to block.

: body
    response body. If #read_body has been called, this method returns
    arg of #read_body DEST. Else gets body as String and returns it.

=end

require 'net/protocol'
require 'uri'


module Net

  class HTTPBadResponse < StandardError; end
  class HTTPHeaderSyntaxError < StandardError; end


  class HTTP < Protocol

    HTTPVersion = '1.1'


    #
    # for backward compatibility
    #

    @@newimpl = true

    def HTTP.version_1_2
      @@newimpl = true
    end

    def HTTP.version_1_1
      @@newimpl = false
    end

    def HTTP.is_version_1_2?
      @@newimpl
    end

    def HTTP.setimplversion( obj )
      f = @@newimpl
      obj.instance_eval { @newimpl = f }
    end
    private_class_method :setimplversion


    #
    # short cut methods
    #

    def HTTP.get_print( arg1, arg2 = nil, port = nil )
      if arg2
        addr, path = arg1, arg2
      else
        uri = arg1
        addr = uri.host
        path = uri.request_uri
        port = uri.port
      end
      new(addr, port || HTTP.default_port).start {|http|
          http.get path, nil, $stdout
      }
      nil
    end

    def HTTP.get( arg1, arg2 = nil, arg3 = nil )
      get_response(arg1,arg2,arg3).body
    end

    def HTTP.get_response( arg1, arg2 = nil, arg3 = nil )
      if arg2 then
        get_by_path(arg1, arg2, arg3)
      else
        get_by_uri(arg1)
      end
    end

    def HTTP.get_by_path( addr, path, port = nil )
      new( addr, port || HTTP.default_port ).start {|http|
          return http.request(Get.new(path))
      }
    end
    private_class_method :get_by_path

    def HTTP.get_by_uri( uri )
      # Should we allow this?
      # uri = URI.parse(uri) unless uri.respond_to?(:host)
      new(uri.host, uri.port).start {|http|
          return http.request(Get.new(uri.request_uri))
      }
    end
    private_class_method :get_by_uri


    #
    # connection
    #

    protocol_param :default_port, '80'
    protocol_param :socket_type,  '::Net::InternetMessageIO'

    class << HTTP
      def start( address, port = nil, p_addr = nil, p_port = nil, &block )
        new( address, port, p_addr, p_port ).start( &block )
      end

      alias newobj new

      def new( address, port = nil, p_addr = nil, p_port = nil )
        obj = Proxy(p_addr, p_port).newobj(address, port)
        setimplversion obj
        obj
      end
    end

    def initialize( addr, port = nil )
      super
      @curr_http_version = HTTPVersion
      @seems_1_0_server = false
      @close_on_empty_response = false
    end

    attr_accessor :close_on_empty_response

    private

    def do_start
      conn_socket
    end

    def do_finish
      disconn_socket
    end


    #
    # proxy
    #

    public

    # no proxy
    @is_proxy_class = false
    @proxy_addr = nil
    @proxy_port = nil

    def HTTP.Proxy( p_addr, p_port = nil )
      p_addr or return self

      p_port ||= port()
      delta = ProxyDelta
      proxyclass = Class.new(self)
      proxyclass.module_eval {
          include delta
          # with proxy
          @is_proxy_class = true
          @proxy_address = p_addr
          @proxy_port    = p_port
      }
      proxyclass
    end

    class << HTTP
      def proxy_class?
        @is_proxy_class
      end

      attr_reader :proxy_address
      attr_reader :proxy_port
    end

    def proxy?
      self.class.proxy_class?
    end

    def proxy_address
      self.class.proxy_address
    end

    def proxy_port
      self.class.proxy_port
    end

    alias proxyaddr proxy_address
    alias proxyport proxy_port

    private

    # no proxy

    def conn_address
      address
    end

    def conn_port
      port
    end

    def edit_path( path )
      path
    end

    module ProxyDelta
      private

      # with proxy
    
      def conn_address
        proxy_address
      end

      def conn_port
        proxy_port
      end

      def edit_path( path )
        'http://' + addr_port() + path
      end
    end


    #
    # http operations
    #

    public

    def get( path, initheader = nil, dest = nil, &block )
      res = nil
      request( Get.new(path,initheader) ) {|res|
          res.read_body dest, &block
      }
      unless @newimpl then
        res.value
        return res, res.body
      end

      res
    end

    def head( path, initheader = nil )
      res = request( Head.new(path,initheader) )
      @newimpl or res.value
      res
    end

    def post( path, data, initheader = nil, dest = nil, &block )
      res = nil
      request( Post.new(path,initheader), data ) {|res|
          res.read_body dest, &block
      }
      unless @newimpl then
        res.value
        return res, res.body
      end

      res
    end

    def put( path, data, initheader = nil )
      res = request( Put.new(path,initheader), data )
      @newimpl or res.value
      res
    end


    def request_get( path, initheader = nil, &block )
      request Get.new(path,initheader), &block
    end

    def request_head( path, initheader = nil, &block )
      request Head.new(path,initheader), &block
    end

    def request_post( path, data, initheader = nil, &block )
      request Post.new(path,initheader), data, &block
    end

    def request_put( path, data, initheader = nil, &block )
      request Put.new(path,initheader), data, &block
    end

    alias get2   request_get
    alias head2  request_head
    alias post2  request_post
    alias put2   request_put


    def send_request( name, path, body = nil, header = nil )
      r = HTTPGenericRequest.new( name, (body ? true : false), true,
                                  path, header )
      request r, body
    end


    def request( req, body = nil, &block )
      unless started? then
        start {
            req['connection'] = 'close'
            return request(req, body, &block)
        }
      end
        
      begin_transport req
          req.exec @socket, @curr_http_version, edit_path(req.path), body
          begin
            res = HTTPResponse.read_new(@socket)
          end while HTTPContinue === res
          res.reading_body(@socket, req.response_body_permitted?) {
              yield res if block_given?
          }
      end_transport req, res

      res
    end

    private

    def begin_transport( req )
      if @socket.closed? then
        reconn_socket
      end
      if @seems_1_0_server then
        req['connection'] = 'close'
      end
      if not req.response_body_permitted? and @close_on_empty_response then
        req['connection'] = 'close'
      end
      req['host'] = addr_port()
    end

    def end_transport( req, res )
      @curr_http_version = res.http_version

      if not res.body and @close_on_empty_response then
        D 'Conn close'
        @socket.close
      elsif keep_alive? req, res then
        D 'Conn keep-alive'
        if @socket.closed? then
          D 'Conn (but seems 1.0 server)'
          @seems_1_0_server = true
        end
      else
        D 'Conn close'
        @socket.close
      end
    end

    def keep_alive?( req, res )
      /close/i === req['connection'].to_s            and return false
      @seems_1_0_server                              and return false

      /keep-alive/i === res['connection'].to_s       and return true
      /close/i      === res['connection'].to_s       and return false
      /keep-alive/i === res['proxy-connection'].to_s and return true
      /close/i      === res['proxy-connection'].to_s and return false

      @curr_http_version == '1.1'                    and return true
      false
    end


    #
    # utils
    #

    private

    def addr_port
      address + (port == HTTP.default_port ? '' : ":#{port}")
    end

    def D( msg )
      if @debug_output then
        @debug_output << msg
        @debug_output << "\n"
      end
    end

  end

  HTTPSession = HTTP


  ###
  ### header
  ###

  module HTTPHeader

    def size
      @header.size
    end

    alias length size

    def []( key )
      @header[ key.downcase ]
    end

    def []=( key, val )
      @header[ key.downcase ] = val
    end

    def each_header( &block )
      @header.each( &block )
    end

    alias each each_header

    def each_key( &block )
      @header.each_key( &block )
    end

    def each_value( &block )
      @header.each_value( &block )
    end

    def delete( key )
      @header.delete key.downcase
    end

    def fetch(*args)
      @header.fetch(*args)
    end

    def key?( key )
      @header.key? key.downcase
    end

    def to_hash
      @header.dup
    end

    def canonical_each
      @header.each do |k,v|
        yield canonical(k), v
      end
    end

    def canonical( k )
      k.split('-').collect {|i| i.capitalize }.join('-')
    end

    def range
      s = @header['range'] or return nil
      s.split(',').collect {|spec|
          m = /bytes\s*=\s*(\d+)?\s*-\s*(\d+)?/i.match(spec) or
                  raise HTTPHeaderSyntaxError, "wrong Range: #{spec}"
          d1 = m[1].to_i
          d2 = m[2].to_i
          if    m[1] and m[2] then  d1..d2
          elsif m[1]          then  d1..-1
          elsif          m[2] then -d2..-1
          else
            raise HTTPHeaderSyntaxError, 'range is not specified'
          end
      }
    end

    def range=( r, fin = nil )
      r = (r ... r + fin) if fin

      case r
      when Numeric
        s = r > 0 ? "0-#{r - 1}" : "-#{-r}"
      when Range
        first = r.first
        last = r.last
        if r.exclude_end? then
          last -= 1
        end

        if last == -1 then
          s = first > 0 ? "#{first}-" : "-#{-first}"
        else
          first >= 0 or raise HTTPHeaderSyntaxError, 'range.first is negative' 
          last > 0  or raise HTTPHeaderSyntaxError, 'range.last is negative' 
          first < last or raise HTTPHeaderSyntaxError, 'must be .first < .last'
          s = "#{first}-#{last}"
        end
      else
        raise TypeError, 'Range/Integer is required'
      end

      @header['range'] = "bytes=#{s}"
      r
    end

    alias set_range range=

    def content_length
      s = @header['content-length'] or return nil
      m = /\d+/.match(s) or
              raise HTTPHeaderSyntaxError, 'wrong Content-Length format'
      m[0].to_i
    end

    def chunked?
      s = @header['transfer-encoding']
      (s and /(?:\A|[^\-\w])chunked(?:[^\-\w]|\z)/i === s) ? true : false
    end

    def content_range
      s = @header['content-range'] or return nil
      m = %r<bytes\s+(\d+)-(\d+)/(?:\d+|\*)>i.match(s) or
              raise HTTPHeaderSyntaxError, 'wrong Content-Range format'
      m[1].to_i .. m[2].to_i + 1
    end

    def range_length
      r = self.content_range
      r and r.length
    end

    def basic_auth( acc, pass )
      @header['authorization'] = 'Basic ' + ["#{acc}:#{pass}"].pack('m').strip
    end

  end


  ###
  ### request
  ###

  class HTTPGenericRequest

    include HTTPHeader

    def initialize( m, reqbody, resbody, path, initheader = nil )
      @method = m
      @request_has_body = reqbody
      @response_has_body = resbody
      @path = path

      @header = tmp = {}
      return unless initheader
      initheader.each do |k,v|
        key = k.downcase
        if tmp.key? key then
          $stderr.puts "WARNING: duplicated HTTP header: #{k}" if $VERBOSE
        end
        tmp[ key ] = v.strip
      end
      tmp['accept'] ||= '*/*'
    end

    attr_reader :method
    attr_reader :path

    def inspect
      "\#<#{self.class} #{@method}>"
    end

    def request_body_permitted?
      @request_has_body
    end

    def response_body_permitted?
      @response_has_body
    end

    alias body_exist? response_body_permitted?

    #
    # write
    #

    # internal use only
    def exec( sock, ver, path, body )
      if body then
        check_body_premitted
        send_request_with_body sock, ver, path, body
      else
        request sock, ver, path
      end
    end

    private

    def check_body_premitted
      request_body_permitted? or
          raise ArgumentError, 'HTTP request body is not premitted'
    end

    def send_request_with_body( sock, ver, path, body )
      if block_given? then
        ac = Accumulator.new
        yield ac              # must be yield, DO NOT USE block.call
        data = ac.terminate
      else
        data = body
      end
      @header['content-length'] = data.size.to_s
      @header.delete 'transfer-encoding'

      unless @header['content-type'] then
        $stderr.puts 'Content-Type did not set; using application/x-www-form-urlencoded' if $VERBOSE
        @header['content-type'] = 'application/x-www-form-urlencoded'
      end

      request sock, ver, path
      sock.write data
    end

    def request( sock, ver, path )
      sock.writeline sprintf('%s %s HTTP/%s', @method, path, ver)
      canonical_each do |k,v|
        sock.writeline k + ': ' + v
      end
      sock.writeline ''
    end
  
  end


  class HTTPRequest < HTTPGenericRequest

    def initialize( path, initheader = nil )
      super self.class::METHOD,
            self.class::REQUEST_HAS_BODY,
            self.class::RESPONSE_HAS_BODY,
            path, initheader
    end

  end


  class HTTP

    class Get < HTTPRequest
      METHOD = 'GET'
      REQUEST_HAS_BODY  = false
      RESPONSE_HAS_BODY = true
    end

    class Head < HTTPRequest
      METHOD = 'HEAD'
      REQUEST_HAS_BODY = false
      RESPONSE_HAS_BODY = false
    end

    class Post < HTTPRequest
      METHOD = 'POST'
      REQUEST_HAS_BODY = true
      RESPONSE_HAS_BODY = true
    end

    class Put < HTTPRequest
      METHOD = 'PUT'
      REQUEST_HAS_BODY = true
      RESPONSE_HAS_BODY = true
    end

  end



  ###
  ### response
  ###

  class HTTPResponse
    # predefine HTTPResponse class to allow inheritance

    def self.body_permitted?
      self::HAS_BODY
    end

    def self.exception_type
      self::EXCEPTION_TYPE
    end
  end


  class HTTPUnknownResponse < HTTPResponse
    HAS_BODY = true
    EXCEPTION_TYPE = ProtocolError
  end
  class HTTPInformation < HTTPResponse           # 1xx
    HAS_BODY = false
    EXCEPTION_TYPE = ProtocolError
  end
  class HTTPSuccess < HTTPResponse               # 2xx
    HAS_BODY = true
    EXCEPTION_TYPE = ProtocolError
  end
  class HTTPRedirection < HTTPResponse           # 3xx
    HAS_BODY = true
    EXCEPTION_TYPE = ProtoRetriableError
  end
  class HTTPClientError < HTTPResponse           # 4xx
    HAS_BODY = true
    EXCEPTION_TYPE = ProtoFatalError
  end
  class HTTPServerError < HTTPResponse           # 5xx
    HAS_BODY = true
    EXCEPTION_TYPE = ProtoServerError
  end

  class HTTPContinue < HTTPInformation           # 100
    HAS_BODY = false
  end
  class HTTPSwitchProtocol < HTTPInformation     # 101
    HAS_BODY = false
  end

  class HTTPOK < HTTPSuccess                            # 200
    HAS_BODY = true
  end
  class HTTPCreated < HTTPSuccess                       # 201
    HAS_BODY = true
  end
  class HTTPAccepted < HTTPSuccess                      # 202
    HAS_BODY = true
  end
  class HTTPNonAuthoritativeInformation < HTTPSuccess   # 203
    HAS_BODY = true
  end
  class HTTPNoContent < HTTPSuccess                     # 204
    HAS_BODY = false
  end
  class HTTPResetContent < HTTPSuccess                  # 205
    HAS_BODY = false
  end
  class HTTPPartialContent < HTTPSuccess                # 206
    HAS_BODY = true
  end

  class HTTPMultipleChoice < HTTPRedirection     # 300
    HAS_BODY = true
  end
  class HTTPMovedPermanently < HTTPRedirection   # 301
    HAS_BODY = true
  end
  class HTTPFound < HTTPRedirection              # 302
    HAS_BODY = true
  end
  HTTPMovedTemporarily = HTTPFound
  class HTTPSeeOther < HTTPRedirection           # 303
    HAS_BODY = true
  end
  class HTTPNotModified < HTTPRedirection        # 304
    HAS_BODY = false
  end
  class HTTPUseProxy < HTTPRedirection           # 305
    HAS_BODY = false
  end
  # 306 unused
  class HTTPTemporaryRedirect < HTTPRedirection  # 307
    HAS_BODY = true
  end

  class HTTPBadRequest < HTTPClientError                    # 400
    HAS_BODY = true
  end
  class HTTPUnauthorized < HTTPClientError                  # 401
    HAS_BODY = true
  end
  class HTTPPaymentRequired < HTTPClientError               # 402
    HAS_BODY = true
  end
  class HTTPForbidden < HTTPClientError                     # 403
    HAS_BODY = true
  end
  class HTTPNotFound < HTTPClientError                      # 404
    HAS_BODY = true
  end
  class HTTPMethodNotAllowed < HTTPClientError              # 405
    HAS_BODY = true
  end
  class HTTPNotAcceptable < HTTPClientError                 # 406
    HAS_BODY = true
  end
  class HTTPProxyAuthenticationRequired < HTTPClientError   # 407
    HAS_BODY = true
  end
  class HTTPRequestTimeOut < HTTPClientError                # 408
    HAS_BODY = true
  end
  class HTTPConflict < HTTPClientError                      # 409
    HAS_BODY = true
  end
  class HTTPGone < HTTPClientError                          # 410
    HAS_BODY = true
  end
  class HTTPLengthRequired < HTTPClientError                # 411
    HAS_BODY = true
  end
  class HTTPPreconditionFailed < HTTPClientError            # 412
    HAS_BODY = true
  end
  class HTTPRequestEntityTooLarge < HTTPClientError         # 413
    HAS_BODY = true
  end
  class HTTPRequestURITooLong < HTTPClientError             # 414
    HAS_BODY = true
  end
  HTTPRequestURITooLarge = HTTPRequestURITooLong
  class HTTPUnsupportedMediaType < HTTPClientError          # 415
    HAS_BODY = true
  end
  class HTTPRequestedRangeNotSatisfiable < HTTPClientError  # 416
    HAS_BODY = true
  end
  class HTTPExpectationFailed < HTTPClientError             # 417
    HAS_BODY = true
  end

  class HTTPInternalServerError < HTTPServerError   # 500
    HAS_BODY = true
  end
  class HTTPNotImplemented < HTTPServerError        # 501
    HAS_BODY = true
  end
  class HTTPBadGateway < HTTPServerError            # 502
    HAS_BODY = true
  end
  class HTTPServiceUnavailable < HTTPServerError    # 503
    HAS_BODY = true
  end
  class HTTPGatewayTimeOut < HTTPServerError        # 504
    HAS_BODY = true
  end
  class HTTPVersionNotSupported < HTTPServerError   # 505
    HAS_BODY = true
  end


  class HTTPResponse   # redefine

    CODE_CLASS_TO_OBJ = {
      '1' => HTTPInformation,
      '2' => HTTPSuccess,
      '3' => HTTPRedirection,
      '4' => HTTPClientError,
      '5' => HTTPServerError
    }
    CODE_TO_OBJ = {
      '100' => HTTPContinue,
      '101' => HTTPSwitchProtocol,

      '200' => HTTPOK,
      '201' => HTTPCreated,
      '202' => HTTPAccepted,
      '203' => HTTPNonAuthoritativeInformation,
      '204' => HTTPNoContent,
      '205' => HTTPResetContent,
      '206' => HTTPPartialContent,

      '300' => HTTPMultipleChoice,
      '301' => HTTPMovedPermanently,
      '302' => HTTPFound,
      '303' => HTTPSeeOther,
      '304' => HTTPNotModified,
      '305' => HTTPUseProxy,
      '307' => HTTPTemporaryRedirect,

      '400' => HTTPBadRequest,
      '401' => HTTPUnauthorized,
      '402' => HTTPPaymentRequired,
      '403' => HTTPForbidden,
      '404' => HTTPNotFound,
      '405' => HTTPMethodNotAllowed,
      '406' => HTTPNotAcceptable,
      '407' => HTTPProxyAuthenticationRequired,
      '408' => HTTPRequestTimeOut,
      '409' => HTTPConflict,
      '410' => HTTPGone,
      '411' => HTTPLengthRequired,
      '412' => HTTPPreconditionFailed,
      '413' => HTTPRequestEntityTooLarge,
      '414' => HTTPRequestURITooLong,
      '415' => HTTPUnsupportedMediaType,
      '416' => HTTPRequestedRangeNotSatisfiable,
      '417' => HTTPExpectationFailed,

      '501' => HTTPInternalServerError,
      '501' => HTTPNotImplemented,
      '502' => HTTPBadGateway,
      '503' => HTTPServiceUnavailable,
      '504' => HTTPGatewayTimeOut,
      '505' => HTTPVersionNotSupported
    }


    class << self

      def read_new( sock )
        httpv, code, msg = read_status_line(sock)
        res = response_class(code).new(httpv, code, msg)
        each_response_header(sock) do |k,v|
          if res.key? k then
            res[k] << ', ' << v
          else
            res[k] = v
          end
        end

        res
      end

      private

      def read_status_line( sock )
        str = sock.readline
        m = /\AHTTP(?:\/(\d+\.\d+))?\s+(\d\d\d)\s*(.*)\z/in.match(str) or
                raise HTTPBadResponse, "wrong status line: #{str.dump}"
        m.to_a[1,3]
      end

      def response_class( code )
        CODE_TO_OBJ[code] or
        CODE_CLASS_TO_OBJ[code[0,1]] or
        HTTPUnknownResponse
      end

      def each_response_header( sock )
        while true do
          line = sock.readuntil( "\n", true )   # ignore EOF
          line.sub!( /\s+\z/, '' )              # don't use chop!
          break if line.empty?

          m = /\A([^:]+):\s*/.match(line) or
                  raise HTTPBadResponse, 'wrong header line format'
          yield m[1], m.post_match
        end
      end

    end


    include HTTPHeader

    def initialize( httpv, code, msg )
      @http_version = httpv
      @code         = code
      @message      = msg

      @header = {}
      @body = nil
      @read = false
    end

    attr_reader :http_version
    attr_reader :code
    attr_reader :message
    alias msg message

    def inspect
      "#<#{self.class} #{@code} readbody=#{@read}>"
    end

    #
    # response <-> exception relationship
    #

    def code_type
      self.class
    end

    def error!
      raise error_type().new(@code + ' ' + @message.dump, self)
    end

    def error_type
      self.class::EXCEPTION_TYPE
    end

    def value
      HTTPSuccess === self or error!
    end

    #
    # header (for backward compatibility only; DO NOT USE)
    #

    def response
      self
    end

    alias header response
    alias read_header response

    #
    # body
    #

    # internal use only
    def reading_body( sock, reqmethodallowbody )
      @socket = sock
      @body_exist = reqmethodallowbody && self.class.body_permitted?
      yield
      self.body
      @socket = nil
    end

    def read_body( dest = nil, &block )
      if @read then
        (dest or block) and
                raise IOError, "#{self.class}\#read_body called twice"
        return @body
      end

      to = procdest(dest, block)
      stream_check
      if @body_exist then
        read_body_0 to
        @body = to
      else
        @body = nil
      end
      @read = true

      @body
    end

    alias body read_body
    alias entity read_body

    private

    def read_body_0( dest )
      if chunked? then
        read_chunked dest
      else
        clen = content_length
        if clen then
          @socket.read clen, dest, true   # ignore EOF
        else
          clen = range_length
          if clen then
            @socket.read clen, dest
          else
            @socket.read_all dest
          end
        end
      end
    end

    def read_chunked( dest )
      len = nil
      total = 0

      while true do
        line = @socket.readline
        m = /[0-9a-fA-F]+/.match(line)
        m or raise HTTPBadResponse, "wrong chunk size line: #{line}"
        len = m[0].hex
        break if len == 0
        @socket.read len, dest; total += len
        @socket.read 2   # \r\n
      end
      until @socket.readline.empty? do
        # none
      end
    end

    def stream_check
      @socket.closed? and raise IOError, 'try to read body out of block'
    end

    def procdest( dest, block )
      (dest and block) and
          raise ArgumentError, 'both of arg and block are given for HTTP method'
      if block then
        ReadAdapter.new(block)
      else
        dest || ''
      end
    end

  end



  # for backward compatibility

  module NetPrivate
    HTTPResponse         = ::Net::HTTPResponse
    HTTPGenericRequest   = ::Net::HTTPGenericRequest
    HTTPRequest          = ::Net::HTTPRequest
    HTTPHeader           = ::Net::HTTPHeader
  end
  HTTPInformationCode = HTTPInformation
  HTTPSuccessCode     = HTTPSuccess
  HTTPRedirectionCode = HTTPRedirection
  HTTPRetriableCode   = HTTPRedirection
  HTTPClientErrorCode = HTTPClientError
  HTTPFatalErrorCode  = HTTPClientError
  HTTPServerErrorCode = HTTPServerError
  HTTPResponceReceiver = HTTPResponse

end   # module Net
