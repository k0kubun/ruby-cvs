# Accumulate a running set of results, and report them at the end

XMARSHAL_DUMP_ONLY = true
require "xmarshal"
  
class ResultDisplay

  def initialize(gatherer)
    @gatherer = gatherer
  end

  def reportOn(op)
    XMarshal.dump(@gatherer, op)
  end
end


