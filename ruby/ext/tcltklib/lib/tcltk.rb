# tof

#### tcltk �饤�֥��
####	Sep. 5, 1997	Y. Shigehiro

require "tcltklib"

################

# module TclTk: tcl/tk �Υ饤�֥�����Τ�ɬ�פˤʤ��Τ򽸤᤿���
# (���, ̾�����֤������� module �ˤ���Ȥ�.)
module TclTk

  # ñ�ˤ����˽񤱤кǽ�� 1 �ټ¹Ԥ����Τ�??

  # ����������դ�̾�����ݻ����Ƥ���Ϣ���������������.
  @namecnt = {}

  # ������Хå����ݻ����Ƥ���Ϣ���������������.
  @callback = {}
end

# TclTk.mainloop(): TclTkLib.mainloop() ��Ƥ�.
def TclTk.mainloop()
  print("mainloop: start\n") if $DEBUG
  TclTkLib.mainloop()
  print("mainloop: end\n") if $DEBUG
end

# TclTk.deletecallbackkey(ca): ������Хå��� TclTk module ���������.
#     tcl/tk ���󥿥ץ꥿�ˤ����ƥ�����Хå������ä����櫓�ǤϤʤ�.
#     ����򤷤ʤ���, �Ǹ�� TclTkInterpreter �� GC �Ǥ��ʤ�.
#     (GC �������ʤ����, �̤�, ����򤷤ʤ��Ƥ��ɤ�.)
#   ca: ������Хå�(TclTkCallback)
def TclTk.deletecallbackkey(ca)
  print("deletecallbackkey: ", ca.to_s(), "\n") if $DEBUG
  @callback.delete(ca.to_s)
end

# TclTk.dcb(ca, wid, W): ��������äƤ���ʣ���Υ�����Хå����Ф���
#     TclTk.deletecallbackkey() ��Ƥ�.
#     �ȥåץ�٥�� <Destroy> ���٥�ȤΥ�����Хå��Ȥ��ƸƤ֤���Τ��.
#   ca: ������Хå�(TclTkCallback) �� Array
#   wid: �ȥåץ�٥�Υ��������å�(TclTkWidget)
#   w: ������Хå��� %W ��Ϳ������, ������ɥ��˴ؤ���ѥ�᡼��(String)
def TclTk.dcb(ca, wid, w)
  if wid.to_s() == w
    ca.each{|i|
      TclTk.deletecallbackkey(i)
    }
  end
end

# TclTk._addcallback(ca): ������Хå�����Ͽ����.
#   ca: ������Хå�(TclTkCallback)
def TclTk._addcallback(ca)
  print("_addcallback: ", ca.to_s(), "\n") if $DEBUG
  @callback[ca.to_s()] = ca
end

# TclTk._callcallback(key, arg): ��Ͽ����������Хå���ƤӽФ�.
#   key: ������Хå������򤹤륭�� (TclTkCallback �� to_s() ���֤���)
#   arg: tcl/tk ���󥿥ץ꥿����Υѥ�᡼��
def TclTk._callcallback(key, arg)
  print("_callcallback: ", @callback[key].inspect, "\n") if $DEBUG
  @callback[key]._call(arg)
  # ������Хå�������֤��ͤϤɤ����ΤƤ���.
  # String ���֤��ʤ���, rb_eval_string() �����顼�ˤʤ�.
  return ""
end

# TclTk._newname(prefix): ��դ�̾��(String)�����������֤�.
#   prefix: ̾������Ƭ��
def TclTk._newname(prefix)
  # ��������̾���Υ����󥿤� @namecnt �����äƤ���Τ�, Ĵ�٤�.
  if !@namecnt.key?(prefix)
    # ���ƻȤ���Ƭ��ʤΤǽ��������.
    @namecnt[prefix] = 1
  else
    # �Ȥä����ȤΤ�����Ƭ��ʤΤ�, ����̾���ˤ���.
    @namecnt[prefix] += 1
  end
  return "#{prefix}#{@namecnt[prefix]}"
end

################

# class TclTkInterpreter: tcl/tk �Υ��󥿥ץ꥿
class TclTkInterpreter

  # initialize(): �����.
  def initialize()
    # ���󥿥ץ꥿����������.
    @ip = TclTkIp.new()

    # ���󥿥ץ꥿�� ruby_fmt ���ޥ�ɤ��ɲä���.
    # ruby_fmt ���ޥ�ɤȤ�, ���ΰ����� format ���ޥ�ɤǽ�������
    # ruby ���ޥ�ɤ��Ϥ���ΤǤ���.
    # (�ʤ�, ruby ���ޥ�ɤ�, ������ 1 �Ĥ����Ȥ�ʤ�.)
    if $DEBUG
      @ip._eval("proc ruby_fmt {fmt args} { puts \"ruby_fmt: $fmt $args\" ; ruby [format $fmt $args] }")
    else
      @ip._eval("proc ruby_fmt {fmt args} { ruby [format $fmt $args] }")
    end

    # @ip._get_eval_string(*args): tcl/tk ���󥿥ץ꥿��ɾ������
    #     ʸ����(String)�����������֤�.
    #   *args: tcl/tk ��ɾ�����륹����ץ�(���б����륪�֥���������)
    def @ip._get_eval_string(*args)
      argstr = ""
      args.each{|arg|
	argstr += " " if argstr != ""
	# �⤷ to_eval() �᥽�åɤ�
	if (arg.respond_to?(:to_eval))
	  # �������Ƥ���Ф����Ƥ�.
	  argstr += arg.to_eval()
	else
	  # �������Ƥ��ʤ���� to_s() ��Ƥ�.
	  argstr += arg.to_s()
	end
      }
      return argstr
    end

    # @ip._eval_args(*args): tcl/tk ���󥿥ץ꥿��ɾ����,
    #     ���η��(String)���֤�.
    #   *args: tcl/tk ��ɾ�����륹����ץ�(���б����륪�֥���������)
    def @ip._eval_args(*args)
      # ���󥿥ץ꥿��ɾ������ʸ��������.
      argstr = _get_eval_string(*args)

      # ���󥿥ץ꥿��ɾ������.
      print("_eval: \"", argstr, "\"") if $DEBUG
      res = _eval(argstr)
      if $DEBUG
	print(" -> \"", res, "\"\n")
      elsif  _return_value() != 0
	print(res, "\n")
      end
      fail(%Q/can't eval "#{argstr}"/) if _return_value() != 0
      return res
    end

    # tcl/tk �Υ��ޥ�ɤ��б����륪�֥������Ȥ�������, Ϣ�����������Ƥ���.
    @commands = {}
    # tcl/tk ���󥿥ץ꥿����Ͽ����Ƥ��뤹�٤ƤΥ��ޥ�ɤ��Ф���,
    @ip._eval("info command").split(/ /).each{|comname|
      if comname =~ /^[.]/
	# ���ޥ�ɤ����������å�(�Υѥ�̾)�ξ���
	# TclTkWidget �Υ��󥹥��󥹤��ä�Ϣ������������.
	@commands[comname] = TclTkWidget.new(@ip, comname)
      else
	# �����Ǥʤ�����
	# TclTkCommand �Υ��󥹥��󥹤��ä�Ϣ������������.
	@commands[comname] = TclTkCommand.new(@ip, comname)
      end
    }
  end

  # commands(): tcl/tk �Υ��ޥ�ɤ��б����륪�֥������Ȥ� Hash ��
  #     ���줿��Τ��֤�.
  def commands()
    return @commands
  end

  # rootwidget(): �롼�ȥ��������å�(TclTkWidget)���֤�.
  def rootwidget()
    return @commands["."]
  end

  # _tcltkip(): @ip(TclTkIp) ���֤�.
  def _tcltkip()
    return @ip
  end

  # method_missing(id, *args): ̤����Υ᥽�åɤ� tcl/tk �Υ��ޥ�ɤȤߤʤ���
  #     �¹Ԥ�, ���η��(String)���֤�.
  #   id: �᥽�åɤΥ���ܥ�
  #   *args: ���ޥ�ɤΰ���
  def method_missing(id, *args)
    # �⤷, �᥽�åɤ� tcl/tk ���ޥ�ɤ�
    if @commands.key?(id.id2name)
      # �����, �¹Ԥ��Ʒ�̤��֤�.
      return @commands[id.id2name].e(*args)
    else
      # ̵����Ф�Ȥ�Ȥν���.
      super
    end
  end
end

# class TclTkObject: tcl/tk �Υ��֥�������
# (���쥯�饹�Ȥ��ƻȤ�.
#  tcltk �饤�֥���Ȥ��ͤ� TclTkObject.new() ���뤳�ȤϤʤ��Ϥ�.)
class TclTkObject

  # initialize(ip, exp): �����.
  #   ip: ���󥿥ץ꥿(TclTkIp)
  #   exp: tcl/tk �Ǥ�ɽ����
  def initialize(ip, exp)
    fail("type is not TclTkIp") if !ip.kind_of?(TclTkIp)
    @ip = ip
    @exp = exp
  end

  # to_s(): tcl/tk �Ǥ�ɽ����(String)���֤�.
  def to_s()
    return @exp
  end
end

# class TclTkCommand: tcl/tk �Υ��ޥ��
# (tcltk �饤�֥���Ȥ��ͤ� TclTkCommand.new() ���뤳�ȤϤʤ��Ϥ�.
#  TclTkInterpreter:initialize() ���� new() �����.)
class TclTkCommand < TclTkObject

  # e(*args): ���ޥ�ɤ�¹Ԥ�, ���η��(String)���֤�.
  #     (e �� exec �ޤ��� eval �� e.)
  #   *args: ���ޥ�ɤΰ���
  def e(*args)
    return @ip._eval_args(to_s(), *args)
  end
end

# class TclTkLibCommand: tcl/tk �Υ��ޥ��
# (�饤�֥��ˤ��¸�����륳�ޥ�ɤ�, tcl/tk ���󥿥ץ꥿�˺ǽ餫��
#  ¸�ߤ��ʤ���Τ�, ���󥿥ץ꥿�� commands() �Ǥ������Ǥ��ʤ�.
#  ���Τ褦�ʤ�Τ��Ф�, ���ޥ�ɤ�̾������ TclTkCommand ���֥������Ȥ�
#  ��������.
class TclTkLibCommand < TclTkCommand

  # initialize(ip, name): �����
  #   ip: ���󥿥ץ꥿(TclTkInterpreter)
  #   name: ���ޥ��̾ (String)
  def initialize(ip, name)
    super(ip._tcltkip, name)
  end
end

# class TclTkVariable: tcl/tk ���ѿ�
class TclTkVariable < TclTkObject

  # initialize(interp, dat): �����.
  #   interp: ���󥿥ץ꥿(TclTkInterpreter)
  #   dat: ���ꤹ����(String)
  #       nil �ʤ�, ���ꤷ�ʤ�.
  def initialize(interp, dat)
    # tcl/tk �Ǥ�ɽ����(�ѿ�̾)��ư��������.
    exp = TclTk._newname("v_")
    # TclTkObject ����������.
    super(interp._tcltkip(), exp)
    # set ���ޥ�ɤ�Ȥ��ΤǤȤäƤ���.
    @set = interp.commands()["set"]
    # �ͤ����ꤹ��.
    set(dat) if dat
  end

  # tcl/tk �� set ��Ȥ���, �ͤ�����/���ȤϤǤ��뤬,
  # ��������ǤϤʤ�ʤΤ�, ���, �᥽�åɤ򤫤֤�����Τ��Ѱդ��Ƥ���.

  # set(data): tcl/tk ���ѿ��� set ���Ѥ����ͤ����ꤹ��.
  #   data: ���ꤹ����
  def set(data)
    @set.e(to_s(), data.to_s())
  end

  # get(): tcl/tk ���ѿ�����(String)�� set ���Ѥ����ɤߤ����֤�.
  def get()
    return @set.e(to_s())
  end
end

# class TclTkWidget: tcl/tk �Υ��������å�
class TclTkWidget < TclTkCommand

  # initialize(*args): �����.
  #   *args: �ѥ�᡼��
  def initialize(*args)
    if args[0].kind_of?(TclTkIp)
      # �ǽ�ΰ����� TclTkIp �ξ��:

      # ���� tcl/tk ���������Ƥ��륦�������åȤ� TclTkWidget �ι�¤��
      # ���֤���. (TclTkInterpreter:initialize() ����Ȥ���.)

      # �ѥ�᡼������ 2 �Ǥʤ���Х��顼.
      fail("illegal # of parameter") if args.size != 2

      # ip: ���󥿥ץ꥿(TclTkIp)
      # exp: tcl/tk �Ǥ�ɽ����
      ip, exp = args

      # TclTkObject ����������.
      super(ip, exp)
    elsif args[0].kind_of?(TclTkInterpreter)
      # �ǽ�ΰ����� TclTkInterpreter �ξ��:

      # �ƥ��������åȤ��鿷���ʥ��������Ȥ���������.

      # interp: ���󥿥ץ꥿(TclTkInterpreter)
      # parent: �ƥ��������å�
      # command: ���������åȤ��������륳�ޥ��(label ��)
      # *args: command ���Ϥ�����
      interp, parent, command, *args = args

      # ���������åȤ�̾������.
      exp = parent.to_s()
      exp += "." if exp !~ /[.]$/
      exp += TclTk._newname("w_")
      # TclTkObject ����������.
      super(interp._tcltkip(), exp)
      # ���������åȤ���������.
      res = @ip._eval_args(command, exp, *args)
#      fail("can't create Widget") if res != exp
      # tk_optionMenu �Ǥ�, �ܥ���̾�� exp �ǻ��ꤹ���
      # res �˥�˥塼̾���֤��Τ� res != exp �Ȥʤ�.
    else
      fail("first parameter is not TclTkInterpreter")
    end
  end
end

# class TclTkCallback: tcl/tk �Υ�����Хå�
class TclTkCallback < TclTkObject

  # initialize(interp, pr, arg): �����.
  #   interp: ���󥿥ץ꥿(TclTkInterpreter)
  #   pr: ������Хå���³��(Proc)
  #   arg: pr �Υ��ƥ졼���ѿ����Ϥ�ʸ����
  #       tcl/tk �� bind ���ޥ�ɤǤϥѥ�᡼���������뤿��� % �ִ���
  #       �Ѥ��뤬, pr �������� % ��񤤤Ƥ⤦�ޤ������ʤ�.
  #       arg ��ʸ�����񤤤Ƥ�����, �����ִ���̤�, pr ��
  #       ���ƥ졼���ѿ����̤��Ƽ�����뤳�Ȥ��Ǥ���.
  #       scrollbar ���ޥ�ɤ� -command ���ץ����Τ褦��
  #       ������ꤷ�ʤ��Ƥ�ѥ�᡼�����դ����ޥ�ɤ��Ф��Ƥ�,
  #       arg ����ꤷ�ƤϤʤ�ʤ�.
  def initialize(interp, pr, arg = nil)
    # tcl/tk �Ǥ�ɽ����(�ѿ�̾)��ư��������.
    exp = TclTk._newname("c_")
    # TclTkObject ����������.
    super(interp._tcltkip(), exp)
    # �ѥ�᡼����ȤäƤ���.
    @pr = pr
    @arg = arg
    # �⥸�塼�����Ͽ���Ƥ���.
    TclTk._addcallback(self)
  end

  # to_eval(): @ip._eval_args ��ɾ������Ȥ���ɽ����(String)���֤�.
  def to_eval()
    if @arg
      # %s �� ruby_fmt ������� bind �ˤ���ִ�����Ƥ��ޤ��Τ�
      # %%s �Ȥ��Ƥ���. �������ä�, ����� bind ����.
      s = %Q/{ruby_fmt {TclTk._callcallback("#{to_s()}", "%%s")} #{@arg}}/
    else
      s = %Q/{ruby_fmt {TclTk._callcallback("#{to_s()}", "%s")}}/
    end

    return s
  end

  # _call(arg): ������Хå���ƤӽФ�.
  #   arg: ������Хå����Ϥ����ѥ�᡼��
  def _call(arg)
    @pr.call(arg)
  end
end

# class TclTkImage: tcl/tk �Υ��᡼��
class TclTkImage < TclTkCommand

  # initialize(interp, t, *args): �����.
  #     ���᡼���������� TclTkImage.new() �ǹԤ���,
  #     �˲��� image delete �ǹԤ�. (���ޤ��������ɻ�����̵��.)
  #   interp: ���󥿥ץ꥿(TclTkInterpreter)
  #   t: ���᡼���Υ����� (photo, bitmap, etc.)
  #   *args: ���ޥ�ɤΰ���
  def initialize(interp, t, *args)
    # tcl/tk �Ǥ�ɽ����(�ѿ�̾)��ư��������.
    exp = TclTk._newname("i_")
    # TclTkObject ����������.
    super(interp._tcltkip(), exp)
    # ���᡼������������.
    res = @ip._eval_args("image create", t, exp, *args)
    fail("can't create Image") if res != exp
  end
end

# eof
