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
    assert_equal(`pwd`, Dir.getwd)
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
    assert_fail("untested")
  end

  def test_s_new
    assert_fail("untested")
  end

  def test_s_open
    assert_fail("untested")
  end

  def test_s_pwd
    assert_fail("untested")
  end

  def test_s_rmdir
    assert_fail("untested")
  end

  def test_s_unlink
    assert_fail("untested")
  end

  def test_close
    assert_fail("untested")
  end

  def test_each
    assert_fail("untested")
  end

  def test_read
    assert_fail("untested")
  end

  def test_rewind
    assert_fail("untested")
  end

  def test_seek
    assert_fail("untested")
  end

  def test_tell
    assert_fail("untested")
  end

end

Rubicon::handleTests(TestDir) if $0 == __FILE__
