#!/usr/bin/env ruby
# $Id$
#
# This file is part of Rubicon, a set of regression tests for the Ruby
# language and its built-in classes and modules.
#
# Initial development by Dave Thomas and Andy Hunt.
#
# Copyright (c) 2000 The Pragmatic Programmers, LLC (www.pragmaticprogrammer.com)
# Distributed according to the terms specified in the Ruby distribution README file.
#

require '../rubicon'

class TestFloats < Rubicon::TestCase

end

# Run these tests if invoked directly

Rubicon::handleTests(TestFloats) if $0 == __FILE__
