# -*- ruby -*-

require 'dl'
require 'dl/import'

module DL
  module Importable
    module Internal
      def define_struct(contents)
	init_types()
	Struct.new(@types, contents)
      end

      def define_union(contents)
	init_types()
	Union.new(@types, contents)
      end

      class Memory
	def initialize(ptr, names, ty, len, enc, dec)
	  @ptr = ptr
	  @names = names
	  @ty    = ty
	  @len   = len
	  @enc   = enc
	  @dec   = dec

	  # define methods
	  @names.each{|name|
	    instance_eval [
	      "def #{name}",
	      "  v = @ptr[\"#{name}\"]",
	      "  v = @dec[\"#{name}\"].call(v,#{@len}) if @dec[\"#{name}\"]",
	      "  return v",
	      "end",
	    ].join("\n")
	  }
	end
      end

      class Struct
	def initialize(types, contents)
	  @names = []
	  @ty   = {}
	  @len  = {}
	  @enc  = {}
	  @dec  = {}
	  @size = 0
	  @tys  = ""
	  @types = types
	  parse(contents)
	end

	def new
	  ptr = DL::malloc(@size)
	  ptr.struct!(@tys, *@names)
	  mem = Memory.new(ptr, @names, @ty, @len, @enc, @dec)
	  return mem
	end

	def parse(contents)
	  contents.each{|elem|
	    name,ty,num,enc,dec = parse_elem(elem)
	    @names.push(name)
	    @ty[name]  = ty
	    @len[name] = num
	    @enc[name] = enc
	    @dec[name] = dec
	    if( num )
	      @tys += "#{ty}#{num}"
	    else
	      @tys += ty
	    end
	  }
	  @size = DL.sizeof(@tys)
	end
	
	def parse_elem(elem)
	  elem.strip!
	  case elem
	  when /^([\w\d_\*]+)([\*\s]+)([\w\d_]+)$/
	    ty = ($1 + $2).strip
	    name = $3
	    num = nil;
	  when /^([\w\d_\*]+)([\*\s]+)([\w\d_]+)\[(\d+)\]$/
	    ty = ($1 + $2).strip
	    name = $3
	    num = $4.to_i
	  else
	    raise(RuntimeError, "invalid element: #{elem}")
	  end
	  ty,_,_,enc,dec = @types.encode_type(ty)
	  return [name,ty,num,enc,dec]
	end
      end  # class Struct
      
      class Union < Struct
	def new
	  ptr = DL::malloc(@size)
	  ptr.union!(@tys, *@names)
	  mem = Memory.new(ptr, @names, @ty, @len, @enc, @dec)
	  return mem
	end
      end
    end  # module Internal
  end  # module Importable
end  # module DL
