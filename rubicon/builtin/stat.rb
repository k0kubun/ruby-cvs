#
# simple routine that uses the external 'checkstat' program
# to return stat values
#

module RubiconStat

  def stat(file)
    res = `../util/checkstat #{file}`.chomp
    raise "unable to stat(#{file})" if $? != 0
    return res.split
  end

  def ctime(file)   stat(file)[10] end
  def blksize(file) stat(file)[11] end

  module_function :stat, :ctime, :blksize
end
