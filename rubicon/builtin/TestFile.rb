require '../rubicon'
require 'stat'

class TestFile < Rubicon::TestCase

  def setup
    setupTestDir

    @file = "_test/_touched"

    #sys("touch -a -t 122512341999 #@file")
    sys_touch("a", "1225", "1234", "1999", @file)
    @aTime = Time.local(1999, 12, 25, 12, 34, 00)

    #sys("touch -m -t 010112341997 #@file")
    sys_touch("m", "0101", "1234", "1997", @file)
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
    sys("touch  #@file")
    ctime = RubiconStat::ctime(@file)
    @cTime = Time.at(ctime)

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
      skipping("$HOME not set")
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
      skipping("~user")
    end
  end

  def test_s_ftype
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail
    system("mkfifo _fifo") # may fail

    {
      "../_test"          => "directory",
      "_file1"            => "file",
      "/dev/tty"          => "characterSpecial",
      "/dev/fd0"          => "blockSpecial",
      "/tmp/.X11-unix/X0" => "socket",
      "_file3"            => "link",
      "_fifo"             => "fifo" 
    }.each { |file, type|
      if File.exists?(file)
        assert_equal(type, File.ftype(file), file.dup)
      else
        skipping("#{type} not supported")
      end
    }
  end

  def test_s_join

    [
      %w( a b c d ),
      %w( a ),
      %w( ),
      %w( a b .. c )
    ].each do |a|
      assert_equal(a.join(File::SEPARATOR), File.join(*a))
    end
  end

  def test_s_link
    Dir.chdir("_test")
    
    assert_equal(0, File.link("_file1", "_file3"))
    
    assert(File.exists?("_file3"))
    assert_equal(2, File.stat("_file1").nlink)
    assert_equal(2, File.stat("_file3").nlink)
    assert(File.stat("_file1").ino == File.stat("_file3").ino)
  end

  def test_s_lstat
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail

    assert_equal(0, File.stat("_file3").size)
    assert(0 < File.lstat("_file3").size)

    assert_equal(0, File.stat("_file1").size)
    assert_equal(0,  File.lstat("_file1").size)
  end

  def test_s_mtime
    assert_equal(@mTime, File.mtime(@file))
  end

  def test_s_open
    file1 = "_test/_file1"

    assert_exception(Errno::ENOENT) { File.open("_gumby") }

    # test block/non block forms
    
    f = File.open(file1)
    begin
      assert_equal(File, f.type)
    ensure
      f.close
    end

    assert_nil(File.open(file1) { |f| assert_equal(File, f.type)})

    # test modes

    modes = [
      %w( r w r+ w+ a a+ ),
      [ File::RDONLY, 
        File::WRONLY | File::CREAT,
        File::RDWR,
        File::RDWR   + File::TRUNC + File::CREAT,
        File::WRONLY + File::APPEND + File::CREAT,
        File::RDWR   + File::APPEND + File::CREAT
        ]]

    for modeset in modes
      sys("rm -f #{file1}")
      sys("touch #{file1}")

      mode = modeset.shift      # "r"

      # file: empty
      File.open(file1, mode) { |f| 
        assert_nil(f.gets)
        assert_exception(IOError) { f.puts "wombat" }
      }

      mode = modeset.shift      # "w"

      # file: empty
      File.open(file1, mode) { |f| 
        assert_nil(f.puts "wombat")
        assert_exception(IOError) { f.gets }
      }

      mode = modeset.shift      # "r+"

      # file: wombat
      File.open(file1, mode) { |f| 
        assert_equal("wombat\n", f.gets)
        assert_nil(f.puts "koala")
        f.rewind
        assert_equal("wombat\n", f.gets)
        assert_equal("koala\n", f.gets)
      }

      mode = modeset.shift      # "w+"

      # file: wombat/koala
      File.open(file1, mode) { |f| 
        assert_nil(f.gets)
        assert_nil(f.puts "koala")
        f.rewind
        assert_equal("koala\n", f.gets)
      }

      mode = modeset.shift      # "a"

      # file: koala
      File.open(file1, mode) { |f| 
        assert_nil(f.puts "wombat")
        assert_exception(IOError) { f.gets }
      }
      
      mode = modeset.shift      # "a+"

      # file: koala/wombat
      File.open(file1, mode) { |f| 
        assert_nil(f.puts "wallaby")
        f.rewind
        assert_equal("koala\n", f.gets)
        assert_equal("wombat\n", f.gets)
        assert_equal("wallaby\n", f.gets)
      }

    end

    # Now try creating files

    filen = "_test/_filen"

    File.open(filen, "w") {}
    assert(File.exists?(filen))
    File.delete(filen)
    
    File.open(filen, "w", 0444) {}
    assert(File.exists?(filen))
    assert(0444, File.stat(filen).mode)
    File.delete(filen)
  end

  def test_s_readlink
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail
    assert_equal("_file1", File.readlink("_file3"))
    assert_exception(Errno::EINVAL) { File.readlink("_file1") }
  end

  def test_s_rename
    Dir.chdir("_test")
    assert_exception(Errno::ENOENT) { File.rename("gumby", "pokey") }
    assert_equal(0, File.rename("_file1", "_renamed"))
    assert(!File.exists?("_file1"))
    assert(File.exists?("_renamed"))

  end

  def test_s_size
    file = "_test/_file1"
    assert_exception(Errno::ENOENT) { File.size("gumby") }
    assert_equal(0, File.size(file))
    File.open(file, "w") { |f| f.puts "123456789" }
    assert_equal(10, File.size(file))
  end

  def test_s_split
    %w{ "/", "/tmp", "/tmp/a", "/tmp/a/b", "/tmp/a/b/", "/tmp//a",
        "/tmp//"
    }.each { |file|
      assert_equals( [ File.dirname(file), File.basename(file) ],
                     File.split(file), file )
    }
  end

  # Stat is pretty much tested elsewhere, so we're minimal here
  def test_s_stat
    assert_instance_of(File::Stat, File.stat("."))
  end


  def test_s_symlink
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail
    assert(File.symlink?("_file3"))
    assert(!File.symlink?("_file1"))
  end

  def test_s_truncate
    file = "_test/_file1"
    File.open(file, "w") { |f| f.puts "123456789" }
    assert_equal(10, File.size(file))
    File.truncate(file, 5)
    assert_equal(5, File.size(file))
    File.open(file, "r") { |f|
      assert_equal("12345", f.read(99))
      assert(f.eof?)
    }
  end

  def myUmask
    Integer(`sh -c umask`.chomp)
  end

  def test_s_umask
    orig = myUmask
    assert_equal(myUmask, File.umask)
    assert_equal(myUmask, File.umask(0404))
    assert_equal(0404, File.umask(orig))
  end

  def test_s_unlink
    Dir.chdir("_test")
    assert_equal(0, File.unlink)
    assert_exception(Errno::ENOENT) { File.unlink("gumby") }
    assert_equal(2, File.unlink("_file1", "_file2"))
  end

  def test_s_utime
    Dir.chdir("_test")

    [ [ 0,                      0 ],
      [ Time.at(0),             Time.at(12345) ],
      [ Time.at(Time.now.to_i), Time.at(54321) ],
      [ Time.at(121314),        Time.now.to_i ]
    ].each { |aTime, mTime|
      File.utime(aTime, mTime, "_file1", "_file2")

      for file in [ "_file1", "_file2" ]
        assert_equal(aTime, File.stat(file).atime) # does automatic conversion
        assert_equal(mTime, File.stat(file).mtime)
      end
    }
  end

  # Instance methods

  def test_atime
    File.open(@file) { |f| assert_equal(@aTime, f.atime) }
  end

  def test_chmod
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

  def test_chown
    super_user
  end

  def test_ctime
    sys("touch  #@file")
    ctime = RubiconStat::ctime(@file)
    @cTime = Time.at(ctime)

    File.open(@file) { |f| assert_equal(@cTime, f.ctime) }
  end

  def test_flock
    Dir.chdir("_test")

    # parent forks, then waits for a SIGUSR1 from child. Child locks file
    # and signals parent, then sleeps
    # When parent gets signal, confirms file si locked, kills child,
    # and confirms its unlocked

    pid = fork
    if pid
      File.open("_file1") { |f|
        trap("USR1") {
          assert_equal(false, f.flock(File::LOCK_EX | File::LOCK_NB))
          Process.kill "KILL", pid
          Process.waitpid(pid, 0)
          assert_equal(0, f.flock(File::LOCK_EX | File::LOCK_NB))
          return
        }
        sleep 10
        assert_fail("Never got signalled")
      }
    else
      File.open("_file1") { |f|
        assert_equal(0, f.flock(File::LOCK_EX))
        Process.kill "USR1", Process.ppid
        sleep 10
        assert_fail "Parent never killed us"
      }
    end
  end

  def test_lstat
    Dir.chdir("_test")
    File.symlink("_file1", "_file3") # may fail

    f1 = File.open("_file1")
    f3 = File.open("_file3")

    assert_equal(0, f3.stat.size)
    assert(0 < f3.lstat.size)

    assert_equal(0, f1.stat.size)
    assert_equal(0, f1.lstat.size)
  end

  def test_mtime
    File.open(@file) { |f| assert_equal(@mTime, f.mtime) }
  end

  def test_path
    File.open(@file) { |f| assert_equal(@file, f.path) }
  end

  def test_truncate
    file = "_test/_file1"
    File.open(file, "w") { |f|
      f.syswrite "123456789" 
      f.truncate(5)
    }
    assert_equal(5, File.size(file))
    File.open(file, "r") { |f|
      assert_equal("12345", f.read(99))
      assert(f.eof?)
    }
  end


end

Rubicon::handleTests(TestFile) if $0 == __FILE__
