require '../rubicon'


class TestFile < Rubicon::TestCase

  def setup
    setupTestDir

    @file = "_test/_file1"

    sys("touch -a -t 122512341999 #@file")
    @aTime = Time.local(1999, 12, 25, 12, 34, 00)

#    sys("touch -c -t 070412341998 #@file")
#    @cTime = Time.local(1998,  7,  4, 12, 34, 00)

    sys("touch -m -t 010112341997 #@file")
    @mTime = Time.local(1997,  1,  1, 12, 34, 00)
  end

  def teardown
    teardownTestDir
  end

  def test_s_atime
    assert_equal(@aTime, File.atime(@file))
  end

  def test_s_basename
    assert_fail("untested")
  end

  def test_s_chmod
    assert_fail("untested")
  end

  def test_s_chown
    assert_fail("untested")
  end

  def test_s_ctime
    assert_fail("untested")
  end

  def test_s_delete
    assert_fail("untested")
  end

  def test_s_dirname
    assert_fail("untested")
  end

  def test_s_expand_path
    assert_fail("untested")
  end

  def test_s_ftype
    assert_fail("untested")
  end

  def test_s_join
    assert_fail("untested")
  end

  def test_s_link
    assert_fail("untested")
  end

  def test_s_lstat
    assert_fail("untested")
  end

  def test_s_mtime
    assert_equal(@mTime, File.mtime(@file))
  end

  def test_s_open
    assert_fail("untested")
  end

  def test_s_readlink
    assert_fail("untested")
  end

  def test_s_rename
    assert_fail("untested")
  end

  def test_s_size
    assert_fail("untested")
  end

  def test_s_split
    assert_fail("untested")
  end

  def test_s_stat
    assert_fail("untested")
  end

  def test_s_symlink
    assert_fail("untested")
  end

  def test_s_truncate
    assert_fail("untested")
  end

  def test_s_umask
    assert_fail("untested")
  end

  def test_s_unlink
    assert_fail("untested")
  end

  def test_s_utime
    assert_fail("untested")
  end

  def test_atime
    assert_fail("untested")
  end

  def test_chmod
    assert_fail("untested")
  end

  def test_chown
    assert_fail("untested")
  end

  def test_ctime
    assert_fail("untested")
  end

  def test_flock
    assert_fail("untested")
  end

  def test_lstat
    assert_fail("untested")
  end

  def test_mtime
    assert_fail("untested")
  end

  def test_path
    assert_fail("untested")
  end

  def test_truncate
    assert_fail("untested")
  end


end

Rubicon::handleTests(TestFile) if $0 == __FILE__
