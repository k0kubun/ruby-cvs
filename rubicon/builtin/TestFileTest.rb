require 'stat'
require 'FileInfoTest.rb'
require 'socket'

class TestFileTest < FileInfoTest


  # compares modified times
  def test_CMP # '<=>'
    assert(test(?=, @file1, @file1))
    assert(test(?=, @file2, @file2))
    assert(test(?>, @file1, @file2))
    assert(test(?<, @file2, @file1))
  end

  def test_hlink
    File.link(@file1, "_link")
    begin
      assert(!test(?-, @file1,  @file2))
      assert(test(?-,  @file1,  @file1))
      assert(test(?-,  @file1, "_link"))
      assert(test(?-,  "_link", @file1))
    ensure
      File.unlink("_link")
    end
  end

  def test_skipped 
    skipping "Tests for O R W X "
  end

  def test_test
    atime = Time.at(RubiconStat::atime(@file1))
    ctime = Time.at(RubiconStat::ctime(@file1))
    mtime = Time.at(RubiconStat::mtime(@file1))

    fileg = "_test/_fileg"
    File.open(fileg, File::CREAT, 02644) { }
    
    filek = "_test/_filek"
    Dir.mkdir(filek, 01644)
    File.chmod(01644, filek)

    filel = "_test/_filel"
    File.symlink(@file1, filel)
    
    filep = "_test/_filep"
    system "mkfifo #{filep}"
    assert_equal(0, $?)

    filer = "_test/_filer"
    File.open(filer, File::CREAT, 0222) { }
    
    fileu = "_test/_fileu"
    File.open(fileu, File::CREAT, 04644) { }
    
    filew = "_test/_filew"
    File.open(filew, File::CREAT, 0444) { }
    
    filez = "_test/_filez"
    File.open(filez, File::CREAT|File::WRONLY, 0644) { |f| f.puts "hi" }

    filesock = "_test/_filesock"
    sock = UNIXServer.open(filesock)
    filesock = nil unless sock

    begin
    
#    system "ls -l _test"

      tests = [
        [ nil,          ?A,    @file1,              atime ],
        [ :blockdev?,   ?b,    "/dev/tty",          false ],
        [ :blockdev?,   ?b,    ".",                 false ],
        [ :chardev?,    ?c,    "/dev/tty",          true  ],
        [ :chardev?,    ?c,    ".",                 false ],
        [ nil,          ?C,    @file1,              ctime ],
        [ :directory?,  ?d,    "/dev/tty",          false ],
        [ :directory?,  ?d,    ".",                 true  ],
        [ :directory?,  ?d,    "/dev/fd0",          false ],
        [ :exist?,      ?e,    filez,               true  ],
        [ :exist?,      ?e,    "/dev/tty",          true  ],
        [ :exist?,      ?e,    "wombat",            false ],
        [ :file?,       ?f,    "/dev/tty",          false ],
        [ :file?,       ?f,    ".",                 false ],
        [ :file?,       ?f,    "/dev/fd0",          false ],
        [ :file?,       ?f,    @file1,              true  ],
        [ :setgid?,     ?g,    @file1,              false ],
        [ :setgid?,     ?g,    fileg,               true  ],
        [ :sticky?,     ?k,    ".",                 false ],
        [ :sticky?,     ?k,    "/dev/tty",          false ],
        [ :sticky?,     ?k,    @file1,              false ],
        [ :sticky?,     ?k,    filek,               true  ],
        [ :symlink?,    ?l,    ".",                 false ],
        [ :symlink?,    ?l,    "/dev/tty",          false ],
        [ :symlink?,    ?l,    @file1,              false ],
        [ :symlink?,    ?l,    filel,               true  ],
        [ nil,          ?M,    @file1,              mtime ],
        [ :owned?,      ?o,    @file1,              true  ],
        [ :owned?,      ?o,    "/etc/passwd",       false ],
        [ :pipe?,       ?p,    "/dev/tty",          false ],
        [ :pipe?,       ?p,    ".",                 false ],
        [ :pipe?,       ?p,    filep,               true  ],
        [ :readable?,   ?r,    @file1,              true  ],
        [ :readable?,   ?r,    filer,               false ],
        [ :size?,       ?s,    filez,               3     ],
        [ :size?,       ?s,    @file2,              nil   ],
        [ :socket?,     ?S,    "/dev/tty",          false ],
        [ :socket?,     ?S,    ".",                 false ],
        [ :socket?,     ?S,    @file1,              false ],
        [ :socket?,     ?S,    filesock,            true  ],
        [ :setuid?,     ?u,    @file1,              false ],
        [ :setuid?,     ?u,    fileu,               true  ],
        [ :writable?,   ?w,    filew,               false ],
        [ :writable?,   ?w,    @file2,              true  ],
        [ :executable?, ?x,    "/dev/tty",          false ],
        [ :executable?, ?x,    "/bin/echo",         true  ],
        [ :executable?, ?x,    "/dev/fd0",          false ],
        [ :zero?,       ?z,    filez,               false ],
        [ :zero?,       ?z,    @file2,              true  ],
      ]

      if $os == Linux
        tests << [ :chardev?,    ?c,    "/dev/fd0",          false ]
        tests << [ :blockdev?,   ?b,    "/dev/fd0",          true  ]
        tests << [ :grpowned?,   ?G,    @file1,              true  ]
        tests << [ :grpowned?,   ?G,    "/etc/passwd",       false ]
      end

      for meth, t, file, result in tests
        if file
          assert_equal(result, test(t, file), "test(?#{t}, #{file})")
          if meth
            assert_equal(result, FileTest.send(meth, file), 
                         "FileTest.#{meth}(#{file})")
          end
        end
      end

    ensure
      sock.close
    end
  end
end

Rubicon::handleTests(TestFileTest) if $0 == __FILE__
