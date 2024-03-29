$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'


class TestSymbol < Rubicon::TestCase

# v---------- test --------------v
  class Fred
    $f1 = :Fred
    def Fred
      $f3 = :Fred
    end
  end
  
  module Test
    Fred = 1
    $f2 = :Fred
  end
  
# ^----------- test ------------^

  Fred.new.Fred

  def test_00sanity
    assert_equals($f1.id,$f2.id)
    assert_equals($f2.id,$f3.id)
  end

  def test_id2name
    assert_equals("Fred",:Fred.id2name)
    assert_equals("Barney",:Barney.id2name)
    assert_equals("wilma",:wilma.id2name)
  end

  def test_to_i
    assert_equals($f1.to_i,$f2.to_i)
    assert_equals($f2.to_i,$f3.to_i)
    assert(:wilma.to_i != :Fred.to_i)
    assert(:Barney.to_i != :wilma.to_i)
  end

  def test_to_s
    assert_equals("Fred",:Fred.id2name)
    assert_equals("Barney",:Barney.id2name)
    assert_equals("wilma",:wilma.id2name)
  end

  def test_type
    assert_equals(Symbol, :Fred.class)
    assert_equals(Symbol, :fubar.class)
  end

  def test_taint
    assert_same(:Fred, :Fred.taint)
    assert(! :Fred.tainted?)
  end

  def test_freeze
    assert_same(:Fred, :Fred.freeze)
    assert(! :Fred.frozen?)
  end

  def test_dup
    assert_exception(TypeError) { :Fred.clone }
    assert_exception(TypeError) { :Fred.dup }
  end
end

Rubicon::handleTests(TestSymbol) if $0 == __FILE__
