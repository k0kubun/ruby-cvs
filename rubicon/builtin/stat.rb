#
# simple routine that uses the external 'checkstat' program
# to return stat values
#

module RubiconStat

  def stat(file)
    res = `../util/checkstat #{file}`.chomp
    raise "unable to stat(#{file}:)" if $? != 0
    return res.split
  end

  def ctime(file)   self.stat(file)[12].to_i end
  def blksize(file) self.stat(file)[11].to_i end

  module_function :stat, :ctime, :blksize
end
