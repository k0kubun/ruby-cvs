#!/usr/bin/env ruby

$: << File.dirname($0) << File.join(File.dirname($0), "..")
require "HashBase.rb"


class TestHash < HashBase

  def initialize(*args)
    @cls = Hash
    super
  end

end

Rubicon::handleTests(TestHash) if $0 == __FILE__
