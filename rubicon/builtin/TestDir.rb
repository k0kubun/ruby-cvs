require '../rubicon'


class TestDir < Rubicon::TestCase

  def sys(cmd)
    assert(system(cmd), cmd)
    assert_equal(0, $?, "cmd: #{$?}")
  end
    
  def setup
    @start = Dir.getwd
    system("mkdir _test")
    if $? != 0
      $stderr.puts "Cannot run directory test: " + 
                   "will destroy existing directory _test"
      exit(99)
    end
    sys("touch _test/_file1")
    sys("touch _test/_file2")
    @files = %w(. .. _file1 _file2)
  end

  def teardown
    Dir.chdir(@start)
    system("rm -f _test/*")
    system("rmdir _test 2>/dev/null")
  end

  def test_s_aref
    assert_equal(%w( _test ),                     Dir["_test"])
    assert_equal(%w( _test/ ),                    Dir["_test/"])
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["_test/*"])
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["_test/_file*"])
    assert_equal(%w(  ),                          Dir["_test/frog*"])

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["**/_file*"])

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["_test/_file[0-9]*"])
    assert_equal(%w( ),                           Dir["_test/_file[a-z]*"])

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["_test/_file{0,1,2,3}"])
    assert_equal(%w( ),                           Dir["_test/_file{4,5,6,7}"])
    
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["**/_f*[il]l*"])
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir["**/_f*[il]e[0-9]"])
    assert_equal(%w( _test/_file1              ), Dir["**/_f*[il]e[01]"])
    assert_equal(%w( _test/_file1              ), Dir["**/_f*[il]e[01]*"])
    assert_equal(%w( _test/_file1              ), Dir["**/_f*[^ie]e[01]*"])
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
    assert_fail("untested")
  end

  def test_s_delete
    assert_exception(Errno::ENOENT)    { Dir.delete "_wombat" } 
    assert_exception(Errno::ENOTEMPTY) { Dir.delete "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.delete("_test"))
    assert_exception(Errno::ENOENT)    { Dir.delete "_test" } 
  end

  def test_s_entries
    assert_exception(Errno::ENOENT)      { Dir.entries "_wombat" } 
    assert_exception(Errno::ENOENT)      { Dir.entries "_test/file*" } 
    assert_equal(@files, Dir.entries("_test"))
    assert_equal(@files, Dir.entries("_test/."))
    assert_equal(@files, Dir.entries("_test/../_test"))
  end

  def test_s_foreach
    expected = @files
    assert_exception(Errno::ENOENT) { Dir.foreach("_wombat") {}}
    assert(nil != Dir.foreach("_test") { |f|
                   assert_equal(expected.shift, f) } )
  end

  def test_s_getwd
    assert_equal(`pwd`.chomp, Dir.getwd)
  end

  def test_s_glob
    assert_equal(%w( _test ),                     Dir.glob("_test"))
    assert_equal(%w( _test/ ),                    Dir.glob("_test/"))
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("_test/*"))
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file*"))
    assert_equal(%w(  ),                          Dir.glob("_test/frog*"))

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("**/_file*"))

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file[0-9]*"))
    assert_equal(%w( ),                           Dir.glob("_test/_file[a-z]*"))

    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("_test/_file{0,1,2,3}"))
    assert_equal(%w( ),                           Dir.glob("_test/_file{4,5,6,7}"))
    
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("**/_f*[il]l*"))
    assert_equal(%w( _test/_file1 _test/_file2 ), Dir.glob("**/_f*[il]e[0-9]"))
    assert_equal(%w( _test/_file1              ), Dir.glob("**/_f*[il]e[01]"))
    assert_equal(%w( _test/_file1              ), Dir.glob("**/_f*[il]e[01]*"))
    assert_equal(%w( _test/_file1              ), Dir.glob("**/_f*[^ie]e[01]*"))
  end

  def test_s_mkdir
    assert_equal(0, Dir.chdir("_test"))
    assert_equal(0, Dir.mkdir("_lower1"))
    assert(File.stat("_lower1").directory?)
    assert_equal(0, Dir.chdir("_lower1"))
    assert_equal(0, Dir.chdir(".."))
    assert_equal(0, Dir.mkdir("_lower2", 0777))
    $stderr.puts "Anyone think of a way to test permissions?"
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
    assert_exception(Errno::ENOENT)    { Dir.rmdir "_wombat" } 
    assert_exception(Errno::ENOTEMPTY) { Dir.rmdir "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.rmdir("_test"))
    assert_exception(Errno::ENOENT)    { Dir.rmdir "_test" } 
  end

  def test_s_unlink
    assert_exception(Errno::ENOENT)    { Dir.unlink "_wombat" } 
    assert_exception(Errno::ENOTEMPTY) { Dir.unlink "_test" } 
    sys("rm _test/*")
    assert_equal(0, Dir.unlink("_test"))
    assert_exception(Errno::ENOENT)    { Dir.unlink "_test" } 
  end

  def test_close
    d = Dir.new(".")
    d.read
    assert_nil(d.close)
    assert_exception(IOError) { d.read }
  end

  def test_each
    expected = @files
    d = Dir.new("_test")
    assert_equal(d, d.each { |f|
                   assert_equal(expected.shift, f) } )
  end

  def test_read
    d = Dir.new("_test")
    @files.each { |expected| assert_equal(expected, d.read);  }
    assert_nil(d.read)
  end

  def test_rewind
    d = Dir.new("_test")
    @files.each { |expected| assert_equal(expected, d.read); }
    assert_nil(d.read)
    d.rewind
    @files.each { |expected| assert_equal(expected, d.read); }
    assert_nil(d.read)
  end

  def test_seek
    d = Dir.new("_test")
    d.read
    pos = d.tell
    assert_equal(Fixnum, pos.type)
    name = d.read
    assert_equal(d, d.seek(pos))
    assert_equal(name, d.read)
  end

  def test_tell
    d = Dir.new("_test")
    d.read
    pos = d.tell
    assert_equal(Fixnum, pos.type)
    name = d.read
    assert_equal(d, d.seek(pos))
    assert_equal(name, d.read)
  end

end

Rubicon::handleTests(TestDir) if $0 == __FILE__
