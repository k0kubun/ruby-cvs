$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestNestedBlock < Rubicon::TestCase

  def get_binding; binding; end
  def get_proc; proc{yield}; end

  def sub_given?; block_given?; end
  def sub_yield; yield; end
  def sub_test_given?; sub_given?; end
  def sub_test_yield; sub_yield; end

  def test_binding_failure
    assert_exception(ArgumentError) {
      eval "proc{proc}.call", get_binding
    }
  end
  def test_binding_failure_with_arg
    assert_exception(ArgumentError) {
      eval "proc{proc}.call {1}", get_binding
    }
  end
  def test_proc_call
    assert_equal("hello wolrd", get_proc{"hello wolrd"}.call)
  end
  def test_block_given
    assert_equal(false, sub_test_given?)
  end
  def test_yield_failure
    assert_exception(LocalJumpError) {sub_test_yield {}}
  end

end

Rubicon::handleTests(TestNestedBlock) if $0 == __FILE__
