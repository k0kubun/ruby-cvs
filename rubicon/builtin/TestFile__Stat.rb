require 'stat'
require 'FileInfoTest.rb'

class TestFile__Stat < FileInfoTest

  def setup
    super
    @s1 = File.stat(@file1)
    @s2 = File.stat(@file2)
  end

  # compares modified times
  def test_CMP # '<=>'
    assert_equal(0,  @s1 <=> @s1)
    assert_equal(0,  @s2 <=> @s2)
    assert_equal(+1, @s1 <=> @s2)
    assert_equal(-1, @s2 <=> @s1)
  end

  def test_atime
    assert_equal(@aTime1, @s1.atime)
    assert_equal(@aTime2, @s2.atime)
  end

  def test_blksize
    blksize = `perl -e "print((stat('.'))[11])"`.to_i
    if $? != 0 || blksize == 0
      skipping("Couldn't find block size")
    else
      assert_equal(blksize, File.stat('.').blksize)
    end
  end

  def try(sym, file, expected)
    if File.exist?(file)
      s = File.stat(file)
      assert_equal(expected, s.send(sym), "File: #{file}")
    else
      skipping("#{sym}: #{file} not found")
    end
  end

  def test_blockdev?
    try(:blockdev?, "/dev/tty", false)
    try(:blockdev?, ".",        false)
    try(:blockdev?, "/dev/fd0", true)
  end

  def test_blocks
    file = "_test/_size"
    File.open(file, "w") { |f| }
    assert_equal(0, File.stat(file).blocks)
    File.open(file, "w") { |f| f.syswrite 'a'}
    assert(File.stat(file).blocks > 0)
    assert(File.stat(file).blocks < 16)
  end

  def test_chardev?
    try(:chardev?, "/dev/tty", true)
    try(:chardev?, ".",        false)
    try(:chardev?, "/dev/fd0", false)
  end

  def test_ctime
    cTime1 = Time.at(RubiconStat::ctime(@file1))
    assert_equal(cTime1, @s1.ctime)
  end

  def test_dev
#    assert_fail("untested")
  end

  def test_directory?
    try(:directory?, "/dev/tty", false)
    try(:directory?, ".",        true)
    try(:directory?, "/dev/fd0", false)
  end

  def test_executable?
    try(:executable?, "/dev/tty", false)
    try(:executable?, "/bin/echo",true)
    try(:executable?, "/dev/fd0", false)
  end

  def test_executable_real?
#    assert_fail("untested")
  end

  def test_file?
    try(:file?, "/dev/tty", false)
    try(:file?, ".",        false)
    try(:file?, "/dev/fd0", false)
    try(:file?, @file1,     true)
  end

  def test_ftype
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail
    system("mkfifo _fifo") # may fail

    {
      "../_test"          => "directory",
      "_file1"            => "file",
      "/dev/tty"          => "characterSpecial",
      "/dev/fd0"          => "blockSpecial",
      "/tmp/.X11-unix/X0" => "socket",
      "_file3"            => "file",   # try uses stat
      "_fifo"             => "fifo" 
    }.each { |file, type|
      try(:ftype, file, type)
    }
    assert_equal("link", File.lstat("_file3").ftype)
  end

  def test_gid
    assert_equal(Process.gid, @s1.gid)
  end

  def test_grpowned?
    try(:grpowned?, @file1,        true)
    try(:grpowned?, "/etc/passwd", false)
  end

  def test_ino
    Dir.chdir("_test")
    File.link("_file1", "_file3") # may fail
    assert(File.stat("_file1").ino > 0)
    assert(File.stat("_file2").ino > 0)
    assert_equal(File.stat("_file1").ino, File.stat("_file3").ino)
  end

  def test_mode
    Dir.chdir("_test")
    File.open("_file1") { |f|
      assert_equal(0,    f.chmod(0))
      assert_equal(0,    f.stat.mode & 0777)
      assert_equal(0,    f.chmod(0400))
      assert_equal(0400, f.stat.mode & 0777)
      assert_equal(0,    f.chmod(0644))
      assert_equal(0644, f.stat.mode & 0777)
    }
  end

  def test_mtime
    assert_equal(@mTime1, @s1.mtime)
    assert_equal(@mTime2, @s2.mtime)
 end

  def test_nlink
    Dir.chdir("_test")
    File.link("_file1", "_file3") # may fail
    try(:nlink, "_file1", 2)
    try(:nlink, "_file2", 1)
    try(:nlink, "_file3", 2)
  end

  def test_owned?
    try(:owned?, @file1,        true)
    try(:owned?, "/etc/passwd", false)
  end

  def test_pipe?
    try(:pipe?, "/dev/tty", false)
    try(:pipe?, ".",        false)
    IO.popen("-") { |p|
      assert_equal(true, (p ? p : $stdout).stat.pipe?)
    }
  end

  def test_rdev
    # assert_fail("untested")
  end

  def test_readable?
    try(:readable?, @file1, true)
    File.chmod(0222, @file1)
    try(:readable?, @file1, false)
  end

  def test_readable_real?
#    assert_fail("untested")
  end

  def test_setgid?
    try(:setgid?, @file1, false)
    File.chmod(02644, @file1)
    try(:setgid?, @file1, true)
  end

  def test_setuid?
    try(:setuid?, @file1, false)
    File.chmod(04644, @file1)
    try(:setuid?, @file1, true)
  end

  def test_size
    File.open(@file1, "w") { |f| f.syswrite "wombat" }
    try(:size, @file1, 6 )
    try(:size, @file2, 0)
  end

  def test_size?
    File.open(@file1, "w") { |f| f.syswrite "wombat" }
    try(:size?, @file1, 6 )
    try(:size?, @file2, nil)
  end

  def test_socket?
    try(:socket?, "/dev/tty", false)
    try(:socket?, ".",        false)
    try(:socket?, @file1,     false)
    try(:socket?, "/tmp/.X11-unix/X0", true)
  end

  def test_sticky?
    Dir.chdir("_test")
    File.chmod(01644, "_file1")
    try(:sticky?, ".",        false)
    try(:sticky?, "/dev/tty", false)
    try(:sticky?, "_file1",   true)
    try(:sticky?, "_file2",   false)
  end

  def test_symlink?
    Dir.chdir("_test")
    File.symlink("_file1", "_symlink")
    try(:symlink?, ".",        false)
    try(:symlink?, "/dev/tty", false)
    try(:symlink?, "_file1",   false)
    try(:symlink?, "_symlink", false)  # try uses stat
    assert(File.lstat("_symlink").symlink?)
  end

  def test_uid
    assert_equal(Process.uid, @s1.uid)
  end

  def test_writable?
    File.chmod(0444, @file1)
    try(:writable?, @file1, false)
    try(:writable?, @file2, true)
  end

  def test_writable_real?
#    assert_fail("untested")
  end

  def test_zero?
    File.open(@file1, "w") { |f| f.puts "wombat" }
    try(:zero?, @file1, false)
    try(:zero?, @file2, true)
  end

end

Rubicon::handleTests(TestFile__Stat) if $0 == __FILE__
