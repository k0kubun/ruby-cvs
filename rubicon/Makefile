# 
# Rubicon is a set of Ruby confidence tests.
#
#  make test   - run the tests
#  make clean  - tidy up
#
# Dave Thomas/Andy Hunt
# The Pragmatic Progrmmers llc
# www.pragmaticprogrammer.com
# $Id$
# Archive save
#


VPATH = util

UTILSTUFF =	util/checkstat


default:	test


test:		$(UTILSTUFF)
		cd builtin && ruby AllBuiltinTests.rb

tarball:	clean
		tar 	--directory=..		\
			--create		\
			--file=rubicon.tgz 	\
			--compress 		\
			--exclude='CVS' 	\
			--exclude='*/.*'	\
		   rubicon

clean:
		rm -f $(UTILSTUFF)
		find . -name \*~ -print | xargs rm -f