require '../rubicon'


class TestFile < Rubicon::TestCase

  def setup
    setupTestDir

    @file = "_test/_touched"

    sys("touch  #@file")
    # Yes - there's a race condition here...
    n = Time.now
    @cTime = Time.local(n.year, n.month, n.day, n.hour, n.min, n.sec)

    sys("touch -a -t 122512341999 #@file")
    @aTime = Time.local(1999, 12, 25, 12, 34, 00)

    sys("touch -m -t 010112341997 #@file")
    @mTime = Time.local(1997,  1,  1, 12, 34, 00)
  end

  def teardown
    File.delete @file if File.exist?(@file)
    teardownTestDir
  end

  def test_s_atime
    assert_equal(@aTime, File.atime(@file))
  end

  def test_s_basename
    assert_equal("_touched", File.basename(@file))
    assert_equal("tmp", File.basename(File.join("/tmp")))
    assert_equal("",    File.basename(File.join("/tmp/")))
    assert_equal("b",   File.basename(File.join(*%w( g f d s a b))))
    assert_equal("",    File.basename("/"))
  end

  def test_s_chmod
    assert_exception(Errno::ENOENT) { File.chmod(0, "_gumby") }
    assert_equal(0, File.chmod(0))
    Dir.chdir("_test")
    assert_equal(1,    File.chmod(0, "_file1"))
    assert_equal(2,    File.chmod(0, "_file1", "_file2"))
    assert_equal(0,    File.stat("_file1").mode & 0777)
    assert_equal(1,    File.chmod(0400, "_file1"))
    assert_equal(0400, File.stat("_file1").mode & 0777)
    assert_equal(1,    File.chmod(0644, "_file1"))
    assert_equal(0644, File.stat("_file1").mode & 0777)
  end

  def test_s_chown
    super_user
  end

  def test_s_ctime
    assert_equal(@cTime, File.ctime(@file))
  end

  def test_s_delete
    Dir.chdir("_test")
    assert_equal(0, File.delete)
    assert_exception(Errno::ENOENT) { File.delete("gumby") }
    assert_equal(2, File.delete("_file1", "_file2"))
  end

  def test_s_dirname
    assert_equal("/",         File.dirname(File.join("/tmp")))
    assert_equal("/tmp",      File.dirname(File.join("/tmp/")))
    assert_equal("g/f/d/s/a", File.dirname(File.join(*%w( g f d s a b))))
    assert_equal("/",         File.dirname("/"))
  end

  def test_s_expand_path
    base = `pwd`.chomp
    assert_equal(base,                 File.expand_path(''))
    assert_equal(File.join(base, 'a'), File.expand_path('a'))
    b1 = File.join(base.split(File::SEPARATOR)[0..-2])
    assert_equal(b1, File.expand_path('..'))

    assert_equal('/tmp',   File.expand_path('', '/tmp'))
    assert_equal('/tmp/a', File.expand_path('a', '/tmp'))
    assert_equal('/tmp/a', File.expand_path('../a', '/tmp/xxx'))

    home = ENV['HOME']
    if (home)
      assert_equal(home, File.expand_path('~'))
      assert_equal(home, File.expand_path('~', '/tmp/gumby/ddd'))
      assert_equal(File.join(home, 'a'),
                         File.expand_path('~/a', '/tmp/gumby/ddd'))
    else
      $stderr.puts "Skipping test_s_expand_path(~)"
    end

    pw = File.open("/etc/passwd")
    if pw
      begin
        users = pw.readlines
        line = ''
        line = users.pop while users.nitems > 0 and line.length == 0
        if line.length > 0 
          name, home  = line.split(':').indices(0, -2)
          assert_equal(home, File.expand_path("~#{name}"))
          assert_equal(home, File.expand_path("~#{name}", "/tmp/gumby"))
          assert_equal(File.join(home, 'a'),
                             File.expand_path("~#{name}/a", "/tmp/gumby"))
        end
      ensure 
        pw.close
      end
    else
      $stderr.puts "Slipping test_s_expand_path(~user)"
    end
  end

  def test_s_ftype
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail
    system("mkfifo _fifo") # may fail

    {
      "../_test" => "directory",
      "_file1" => "file",
      "/dev/tty"     => "characterSpecial",
      "/dev/fd0"     => "blockSpecial",
      "/tmp/.X11-unix/X0" => "socket",
      "_file3"  => "link",
      "_fifo"   => "fifo" 
    }.each { |file, type|
      if File.exists?(file)
        assert_equal(type, File.ftype(file), file.dup)
      else
        skipping("#{type} not supported")
      end
    }
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
