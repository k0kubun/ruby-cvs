# Weak Reference class that does not bother GCing.
#
# Usage:
#   foo = Object.new
#   foo.hash
#   foo = WeakRef.new(foo)
#   foo.hash
#   ObjectSpace.garbage_collect
#   foo.hash	# => Raises WeakRef::RefError (because original GC'ed)

require "delegate"

class WeakRef<Delegater

  Exception :RefError

  ID_MAP =  {}
  ID_REV_MAP =  {}
  ObjectSpace.add_finalizer(lambda{|id|
			      rid = ID_MAP[id]
			      if rid
				ID_REV_MAP[rid] = nil
				ID_MAP[id] = nil
			      end
			      rid = ID_REV_MAP[id]
			      if rid
				ID_REV_MAP[id] = nil
				ID_MAP[rid] = nil
			      end
			    })
			    
  def initialize(orig)
    super
    @id = orig.id
    ObjectSpace.call_finalizer orig
    ID_MAP[@id] = self.id
    ID_REV_MAP[self.id] = @id
  end

  def __getobj__
    unless ID_MAP[@id]
      $@ = caller(1)
      $! = RefError.new("Illegal Reference - probably recycled")
      raise
    end
    ObjectSpace.id2ref(@id)
#    ObjectSpace.each_object do |obj|
#      return obj if obj.id == @id
#    end
  end

  def weakref_alive?
    if ID_MAP[@id]
      true
    else
      false
    end
  end

  def []
    __getobj__
  end
end

foo = Object.new
p foo.hash
foo = WeakRef.new(foo)
p foo.hash
ObjectSpace.garbage_collect
p foo.hash
