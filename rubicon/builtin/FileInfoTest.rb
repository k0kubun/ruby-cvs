require '../rubicon'

class FileInfoTest < Rubicon::TestCase
  def setup
    setupTestDir

    @file1 = "_test/_touched1"
    @file2 = "_test/_touched2"

    [ @file1, @file2 ].each { |file|
      File.delete file if File.exist?(file)
    }

    #sys("touch -a -t 122512341999 #@file1")
    sys_touch("a", "1225", "1234", "1999", @file1)
    @aTime1 = Time.local(1999, 12, 25, 12, 34, 00)

    #sys("touch -m -t 010112341997 #@file1")
    sys_touch("m", "0101", "1234", "1997", @file1)
    @mTime1 = Time.local(1997,  1,  1, 12, 34, 00)

    # File two is before file 1 in access time, and
    # after in modification time

    #sys("touch -a -t 010212342000 #@file2")
    sys_touch("a", "0102", "1234", "2000", @file2)
    @aTime2 = Time.local(2000, 1, 2, 12, 34, 00)

    sys_touch("m", "0203", "1234", "1995", @file2)
    @mTime2 = Time.local(1995,  2,  3, 12, 34, 00)
  end

  def teardown
    [ @file1, @file2 ].each { |file|
      File.delete file if File.exist?(file)
    }
    teardownTestDir
  end
end
