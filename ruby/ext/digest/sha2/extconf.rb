# $RoughId: extconf.rb,v 1.1 2001/07/13 15:38:27 knu Exp $
# $Id$

require "mkmf"

$CFLAGS << " -DHAVE_CONFIG_H -I$(srcdir)/.."

$objs = [
  "sha2.#{$OBJEXT}",
  "sha2hl.#{$OBJEXT}",
  "sha2init.#{$OBJEXT}",
]

have_header("sys/cdefs.h")

have_header("inttypes.h")

have_header("unistd.h")

if try_run(<<SRC, $defs.join(' '))
#include "../defs.h"
int main(void) {
#ifdef NO_UINT64_T
    return 1;
#else
    return 0;
#endif
}
SRC
then
  create_makefile("digest/sha2")
else
  puts "** Cannot find a 64bit integer type - skipping the SHA2 module."
end
