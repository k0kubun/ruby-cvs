# Accumulate a running set of results, and report them at the end

#
# This is a temporary hack - we have made changes to xmarshal
# to allow rubicon to run without having to have xmlparser 
# installed
#

XMARSHAL_DUMP_ONLY = true
require "rubicon_xmarshal"
  
class ResultDisplay

  def initialize(gatherer)
    @gatherer = gatherer
  end

  def reportOn(op)
    XMarshal.dump(@gatherer, op)
  end
end


