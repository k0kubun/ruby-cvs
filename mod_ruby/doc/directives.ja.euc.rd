=begin

= mod_ruby Apache�ǥ��쥯�ƥ���

[((<�ܼ�|URL:index.ja.jis.html>))
|((<RD����|URL:directives.ja.euc.rd>))]

* ((<RubyAddPath|RubyAddPath directory...>))
* ((<RubyRequire|RubyRequire library...>))
* ((<RubyHandler|RubyHandler expr>))
* ((<RubyPassEnv|RubyPassEnv name...>))
* ((<RubySetEnv|RubySetEnv name val>))
* ((<RubyTimeOut|RubyTimeOut sec>))
* ((<RubySafeLevel|RubySafeLevel level>))
* ((<RubyOutputMode|RubyOutputMode mode>))
* ((<RubyKanjiCode|RubyKanjiCode kcode>))

--- RubyAddPath directory...
      �饤�֥��θ����ѥ��˥ǥ��쥯�ȥ���ɲä��롣

      ��:

        RubyAddPath /home/shugo/ruby

--- RubyRequire library...
      ((|library|))��(({require}))���롣

      ��:

        RubyRequire apache/ruby-run
        RubyRequire cgi

--- RubyHandler expr
      ruby-object�ϥ�ɥ�Ǽºݤ˽�����Ԥ����֥������Ȥ��֤�������ꤹ�롣

      ��:

        <Location /ruby>
          SetHandler ruby-object
          RubyHandler Apache::RubyRun.instance
        </Location>

--- RubyPassEnv name...
      ������ץȤ˼����Ϥ��Ķ��ѿ���̾������ꤹ�롣
      ���Υǥ��쥯�ƥ��֤����ꤵ��ʤ��ä���硢CGI�Ķ��ѿ�(QUERY_STRING�ʤ�)
      �Τߤ�Ruby������ץȤ��Ϥ���롣
      ���Υƥ��쥯�ƥ��֤����ꤵ�줿��硢���٤Ƥ�CGI�Ķ��ѿ��ȡ��ꥹ�Ȥ��줿
      ¾�δĶ��ѿ�����Ruby������ץȤ������Ѳ�ǽ�Ȥʤ롣
      ����������Τߤ˻���ġ�

      ��:

        RubyPassEnv HOSTNAME OSTYPE MACHTYPE

--- RubySetEnv name val
      ������ץȤ˼����Ϥ��Ķ��ѿ����ͤ���ꤹ�롣

      ��:

        RubySetEnv LANG "ja_JP.eucJP"

--- RubyTimeOut sec
      ������ץȤΥ����ॢ���Ȼ���(ñ�̤���)����ꤹ�롣
      �����ॢ���Ȼ��֤��᤮�Ƥ�¹Ԥ���Ƥ��륹����ץȤ϶���Ū��
      ��λ����롣
      ����������Τߤ˻���ġ�

      ��:

        RubyTimeOut 60

--- RubySafeLevel level
      ((|$SAFE|))�Υǥե�����ͤ���ꤹ�롣
      ����������Τߤ˻���ġ�

      $SAFE�ϥ������ƥ���٥�Ǥ��롣
      $SAFE���ͤ�0����4�ޤǤ������Ǥʤ���Фʤ�ʤ���
      mod_ruby�Ǥ�$SAFE�Υǥե�����ͤ�1�Ǥ��롣

      $SAFE��1�ʾ�ξ�硢Ruby�ϥ桼�����������줿�ǡ���������Ū�������Τ���
      �������Ѥ���Τ�ػߤ��롣

      $SAFE��2�ʾ�ξ�硢Ruby�Ϥ���Ǥ�񤭹��ळ�ȤΤǤ����꤫��ץ������
      �ե����������ɤ��뤳�Ȥ�ػߤ��롣
      writable locations.

      $SAFE��3�ʾ�ξ�硢�������������줿���٤ƤΥ��֥������Ȥϱ�������롣

      $SAFE��4�ʾ�ξ�硢Ruby�ϥ������Х��ѿ��ʤɤΥ������Х�ʾ��֤��ѹ���
      �ػߤ��롣

      ��:

        RubySafeLevel 2

--- RubyOutputMode mode
      ������ץȤν��ϥ⡼�ɤ���ꤹ�롣((|mode|))��(({nosync})), (({sync})),
      (({syncheader}))�Τ����Τɤ줫��ĤǤʤ���Фʤ�ʤ���((|mode|))��
      (({nosync}))�ξ�硢������ץȤΤ��٤Ƥν��ϤϥХåե���󥰤��졢�������
      �Ȥμ¹Ԥν�λ���˥ե�å��夵��롣((|mode|))��(({sync}))�ξ�硢�������
      �ȤΤ��٤Ƥν��ϤϤ������˥��饤��Ȥ���������롣((|mode|))��
      (({syncheader}))�ξ�硢�إå��ν��ϤΤߤ����������������졢¾�ν��ϤϥХ�
      �ե���󥰤���롣�ǥե�����ͤ�(({nosync}))��
      
      ��:

        RubyOutputMode syncheader

--- RubyKanjiCode kcode
      Ruby��((|$KCODE|))����ꤹ�롣�ǥե�����ͤ�(({"NONE"}))��

      ��:

        RubyKanjiCode euc

=end