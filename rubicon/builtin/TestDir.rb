$: << File.dirname($0) << File.join(File.dirname($0), "..")
require 'rubicon'

class TestDir < Rubicon::TestCase

  def setup
    setupTestDir
  end

  def teardown
    teardownTestDir
  end

  def test_s_aref
    [
      [ %w( _test ),                     Dir["_test"] ],
      [ %w( _test/ ),                    Dir["_test/"] ],
      [ %w( _test/_file1 _test/_file2 ), Dir["_test/*"] ],
      [ %w( _test/_file1 _test/_file2 ), Dir["_test/_file*"] ],
      [ %w(  ),                          Dir["_test/frog*"] ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir["**/_file*"] ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir["_test/_file[0-9]*"] ],
      [ %w( ),                           Dir["_test/_file[a-z]*"] ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir["_test/_file{0,1,2,3}"] ],
      [ %w( ),                           Dir["_test/_file{4,5,6,7}"] ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir["**/_f*[il]l*"] ],      [ %w( _test/_file1 _test/_file2 ), Dir["**/_f*[il]e[0-9]"] ],
      [ %w( _test/_file1              ), Dir["**/_f*[il]e[01]"] ],
      [ %w( _test/_file1              ), Dir["**/_f*[il]e[01]*"] ],
      [ %w( _test/_file1              ), Dir["**/_f*[^ie]e[01]*"] ],
    ].each do |expected, got|
      assert_set_equal(expected, got)
    end
  end

  def test_s_chdir
    start = Dir.getwd
    assert_exception(Errno::ENOENT)       { Dir.chdir "_wombat" }
    assert_equal(0,                         Dir.chdir("_test"))
    assert_equal(File.join(start, "_test"), Dir.getwd)
    assert_equal(0,                         Dir.chdir(".."))
    assert_equal(start,                     Dir.getwd)
    assert_equal(0,                         Dir.chdir("/tmp"))
    assert_equal("/tmp",                    Dir.getwd)
  end

  def test_s_chroot
    super_user
  end

  def test_s_delete
    assert_kindof_exception(SystemCallError)    { Dir.delete "_wombat" } 
    assert_kindof_exception(SystemCallError)    { Dir.delete "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.delete("_test"))
    assert_kindof_exception(SystemCallError)    { Dir.delete "_test" } 
  end

  def test_s_entries
    assert_exception(Errno::ENOENT)      { Dir.entries "_wombat" } 
    assert_exception(Errno::ENOENT)      { Dir.entries "_test/file*" } 
    assert_set_equal(@files, Dir.entries("_test"))
    assert_set_equal(@files, Dir.entries("_test/."))
    assert_set_equal(@files, Dir.entries("_test/../_test"))
  end

  def test_s_foreach
    got = []
    entry = nil
    assert_exception(Errno::ENOENT) { Dir.foreach("_wombat") {}}
    assert_nil(Dir.foreach("_test") { |f| got << f } )
    assert_set_equal(@files, got)
  end

  def test_s_getwd
    assert_equal(`pwd`.chomp, Dir.getwd)
  end

  def test_s_glob
    [
      [ %w( _test ),                     Dir.glob("_test") ],
      [ %w( _test/ ),                    Dir.glob("_test/") ],
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("_test/*") ],
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file*") ],
      [ %w(  ),                          Dir.glob("_test/frog*") ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("**/_file*") ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file[0-9]*") ],
      [ %w( ),                           Dir.glob("_test/_file[a-z]*") ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file{0,1,2,3}") ],
      [ %w( ),                           Dir.glob("_test/_file{4,5,6,7}") ],
      
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("**/_f*[il]l*") ],
      [ %w( _test/_file1 _test/_file2 ), Dir.glob("**/_f*[il]e[0-9]") ],
      [ %w( _test/_file1              ), Dir.glob("**/_f*[il]e[01]") ],
      [ %w( _test/_file1              ), Dir.glob("**/_f*[il]e[01]*") ],
      [ %w( _test/_file1              ), Dir.glob("**/_f*[^ie]e[01]*") ],
    ].each do |expected, got|
      assert_set_equal(expected, got)
    end
  end

  def test_s_mkdir
    assert_equal(0, Dir.chdir("_test"))
    assert_equal(0, Dir.mkdir("_lower1"))
    assert(File.stat("_lower1").directory?)
    assert_equal(0, Dir.chdir("_lower1"))
    assert_equal(0, Dir.chdir(".."))
    assert_equal(0, Dir.mkdir("_lower2", 0777))
    skipping "Anyone think of a way to test permissions?"
    assert_equal(0, Dir.delete("_lower1"))
    assert_equal(0, Dir.delete("_lower2"))
  end

  def test_s_new
    assert_exception(ArgumentError) { Dir.new }
    assert_exception(ArgumentError) { Dir.new("a", "b") }
    assert_exception(Errno::ENOENT) { Dir.new("_wombat") }

    assert_equal(Dir, Dir.new(".").type)
  end

  def test_s_open
    assert_exception(ArgumentError) { Dir.open }
    assert_exception(ArgumentError) { Dir.open("a", "b") }
    assert_exception(Errno::ENOENT) { Dir.open("_wombat") }

    assert_equal(Dir, Dir.open(".").type)
    assert_nil(Dir.open(".") { |d| assert_equal(Dir, d.type) } )
  end

  def test_s_pwd
    assert_equal(`pwd`.chomp, Dir.pwd)
  end

  def test_s_rmdir
    assert_kindof_exception(SystemCallError)    { Dir.rmdir "_wombat" } 
    assert_kindof_exception(SystemCallError)    { Dir.rmdir "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.rmdir("_test"))
    assert_kindof_exception(SystemCallError)    { Dir.rmdir "_test" } 
  end

  def test_s_unlink
    assert_kindof_exception(SystemCallError)    { Dir.unlink "_wombat" } 
    assert_kindof_exception(SystemCallError)    { Dir.unlink "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.unlink("_test"))
    assert_kindof_exception(SystemCallError)    { Dir.unlink "_test" } 
  end

  def test_close
    d = Dir.new(".")
    d.read
    assert_nil(d.close)
    assert_exception(IOError) { d.read }
  end

  def test_each
    got = []
    d = Dir.new("_test")
    assert_equal(d, d.each { |f| got << f })
    assert_set_equal(@files, got)
  end

  def test_read
    d = Dir.new("_test")
    got = []
    entry = nil
    got << entry while entry = d.read
    assert_set_equal(@files, got)
  end

  def test_rewind
    d = Dir.new("_test")
    entry = nil
    got = []
    got << entry while entry = d.read
    assert_set_equal(@files, got)
    d.rewind
    got = []
    got << entry while entry = d.read
    assert_set_equal(@files, got)
  end

  def test_seek
    d = Dir.new("_test")
    d.read
    pos = d.tell
    assert_equal(Fixnum, pos.type)
    name = d.read
    assert_equal(d, d.seek(pos))
    assert_equal(name, d.read)
    d.close
  end

  def test_tell
    d = Dir.new("_test")
    d.read
    pos = d.tell
    assert_equal(Fixnum, pos.type)
    name = d.read
    assert_equal(d, d.seek(pos))
    assert_equal(name, d.read)
    d.close
  end

end

Rubicon::handleTests(TestDir) if $0 == __FILE__
