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

# Release must have VERSION variable set
#
#    make VERSION=0.1 release
#

release:	clean
		ruby -i -pe 'sub!(/V\d+(\.\d+)+/, "V$(VERSION)") if /VERSION =/' rubicon.rb
		cvs commit
		cvs rtag `echo V$(VERSION) | sed s/\\\\./_/` rubicon
		ln -fs rubicon ../rubicon-$(VERSION)
		tar 	--directory=..			\
			--create			\
			--dereference			\
			--file=rubicon-$(VERSION).tgz 	\
			--compress 			\
			--exclude='CVS' 		\
			--exclude='*/.*'		\
			rubicon-$(VERSION)
		rm ../rubicon-$(VERSION)

clean:
		rm -f $(UTILSTUFF)
		rm -f rubicon*.tgz
		find . -name \*~ -print | xargs rm -f