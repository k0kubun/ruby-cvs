# Copyright (C) 2000  Network Applied Communication Laboratory, Inc.
# Copyright (C) 2000  Information-technology Promotion Agency, Japan

require 'cgi'
require 'final'

class CGI
  class Session

    attr_reader :session_id

    def Session::callback(dbman)
      lambda{
	dbman.close
      }
    end

    def create_new_id
      require 'md5'
      md5 = MD5::new
      md5.update(String(Time::now))
      md5.update(String(rand(0)))
      md5.update(String($$))
      md5.update('foobar')
      @session_id = md5.hexdigest[0,16]
    end
    private :create_new_id

    def initialize(request, option={})
      session_key = option['session_key'] || '_session_id'
      id, = option['session_id']
      unless id
	if option['new_session']
	  id = create_new_id
	end
      end
      unless id
	id, = request[session_key]
	unless id
	  id, = request.cookies[session_key]
	end
	unless id
	  if option.key?('new_session') and not option['new_session']
	    raise ArgumentError, "session_key `%s' should be supplied"%session_key
	  end
	  id = create_new_id
	end
      end
      @session_id = id
      dbman = option['database_manager'] || FileStore
      @dbman = dbman::new(self, option)
      request.instance_eval do
	@output_hidden = {session_key => id}
	@output_cookies =  [
          Cookie::new("name" => session_key,
		      "value" => id,
		      "path" => if ENV["SCRIPT_NAME"] then
				  File::dirname(ENV["SCRIPT_NAME"])
				else
				  ""
				end)
        ]
      end
      ObjectSpace::define_finalizer(self, Session::callback(@dbman))
    end

    def [](key)
      unless @data
	@data = @dbman.restore
      end
      @data[key]
    end

    def []=(key, val)
      unless @write_lock
	@write_lock = true
      end
      unless @data
	@data = @dbman.restore
      end
      @data[key] = String(val)
    end

    def update
      @dbman.update
    end

    def close
      @dbman.close
    end

    def delete
      @dbman.delete
    end

    class FileStore
      def initialize(session, option={})
	dir = option['tmpdir'] || ENV['TMP'] || '/tmp'
	prefix = option['prefix'] || ''
	path = dir+"/"+prefix+session.session_id
	path.untaint
	unless File::exist? path
	  @hash = {}
	end
	begin
	  @f = open(path, "r+")
	rescue Errno::ENOENT
	  @f = open(path, "w+")
	end
      end

      def restore
	unless @hash
	  @hash = {}
	  @f.flock File::LOCK_EX
	  @f.rewind
	  for line in @f
	    line.chomp!
	    k, v = line.split('=',2)
	    @hash[CGI::unescape(k)] = CGI::unescape(v)
	  end
	end
	@hash
      end

      def update
	@f.rewind
	for k,v in @hash
	  @f.printf "%s=%s\n", CGI::escape(k), CGI::escape(v)
	end
	@f.truncate @f.tell
      end

      def close
	update
	@f.close
      end

      def delete
	path = @f.path
	@f.close
	File::unlink path
      end
    end

    class MemoryStore
      GLOBAL_HASH_TABLE = {}

      def initialize(session, option={})
	@session_id = session.session_id
	GLOBAL_HASH_TABLE[@session_id] = {}
      end

      def restore
	GLOBAL_HASH_TABLE[@session_id]
      end

      def update
	# don't need to update; hash is shared
      end

      def close
	# don't need to close
      end

      def delete
	GLOBAL_HASH_TABLE[@session_id] = nil
      end
    end
  end
end
