$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestGC < Rubicon::TestCase

  def test_garbage_collect
    skipping("need test")
  end

  def test_s_disable
    skipping("need test")
  end

  def test_s_enable
    skipping("need test")
  end

  def test_s_start
    skipping("need test")
  end

end

Rubicon::handleTests(TestGC) if $0 == __FILE__
