# 
# Rubicon is a set of Ruby confidence tests.
#
#  make test   - run the tests
#  make clean  - tidy up
#
# Dave Thomas/Andy Hunt
# The Pragmatic Progrmmers llc
# www.pragmaticprogrammer.com
# $Header$
# $Log$


VPATH = util

UTILSTUFF =	util/checkstat


default:	test


test:		$(UTILSTUFF)
		cd builtin && ruby AllBuiltinTests.rb


clean:
		rm -f $(UTILSTUFF)