=begin

= mod_ruby ���饹��ե����

[((<�ܼ�|URL:index.ja.jis.html>))
|((<RD����|URL:classes.ja.euc.rd>))]

* ((<Apache>))
* ((<Apache::Request>))
* ((<Apache::Table>))

== Apache

Apache�ε�ǽ���󶡤���⥸�塼�롣

=== �⥸�塼��ؿ�

--- server_version
      �����ФΥС����������ɽ��ʸ������֤���

--- server_built
      �����Ф��ӥ�ɤ��줿���դ�ɽ��ʸ������֤���

--- request
      ���ߤΥꥯ�����Ȥ�ɽ��((<Apache::Request>))���֥������Ȥ��֤���

--- unescape_url(str)
      URL���󥳡��ɤ��줿ʸ����Υǥ����ɤ�Ԥ���

[((<�ܼ������|mod_ruby ���饹��ե����>))]

== Apache::Request

request_rec(�ꥯ�����Ⱦ����ɽ������Apache�Υǡ�����)���åפ��륯�饹��

=== �����ѡ����饹

Object

=== ���󥯥롼�ɤ���Ƥ���⥸�塼��

Enumerable

=== �᥽�å�

--- hostname
      �ե�URI��Host:��Ϳ�����줿�ۥ���̾���֤���

--- unparsed_uri
      �ѡ�������Ƥ��ʤ�URI���֤���

--- uri
      URI�Υѥ������֤���

--- filename
      ������ץȤΥե�����̾���֤���

--- path_info
      PATH_INFO���֤���

--- request_time
      �ꥯ�����Ȥγ��ϻ����ɽ��Time���֥������Ȥ��֤���

--- request_method
      �ꥯ�����ȥ᥽�å�(GET, HEAD, POST)���֤���

--- header_only?
      HEAD�ꥯ�����Ȥξ�硢�����֤���

--- args
      �ꥯ�����Ȥΰ������֤���

--- headers_in
      �ꥯ�����Ȥإå���ɽ��((<Apache::Table>))���֥������Ȥ��֤���

--- read([len])
--- gets([rs])
--- readline([rs])
--- readlines([rs])
--- each([rs]) {|line|...}
--- each_line([rs]) {|line|...}
--- each_byte {|ch|...}
--- getc
--- readchar
--- ungetc(ch)
--- tell
--- seek(offset, [whence])
--- rewind
--- pos
--- pos= n
--- eof
--- eof?
--- binmode
      ���饤����Ȥ���Υǡ����������롣
      �ƥ᥽�åɤ�IO��Ʊ̾�Υ᥽�åɤ�Ʊ�ͤ�ư��롣

--- status_line= str
      ���ơ������饤���((|str|))����ꤹ�롣

--- status_line
      ���ꤵ�줿���ơ������饤����֤���

--- headers_out
      �쥹�ݥ󥹤إå���ɽ��((<Apache::Table>))���֥������Ȥ��֤���
      �쥹�ݥ󥹤إå���((<send_http_header>))�ˤ����Ϥ���롣

--- content_type= str
      �쥹�ݥ󥹤إå���Content-Type����ꤹ�롣

--- content_type
      ���ꤵ�줿Content-Type���֤���

--- content_encoding= str
      �쥹�ݥ󥹤إå���Content-Encoding����ꤹ�롣

--- content_encoding
      ���ꤵ�줿Content-Encoding���֤���

--- content_languages= str
      �쥹�ݥ󥹤إå���Content-Languages����ꤹ�롣

--- content_languages
      ���ꤵ�줿Content-Languages���֤���

--- send_http_header
      �쥹�ݥ󥹤إå��򥯥饤����Ȥ��������롣
      ���ʾ�ƤӽФ��Ƥ���٤������Ϥ���ʤ���

--- write(str)
--- putc(ch)
--- print(arg...)
--- printf(fmt, arg...)
--- puts(arg...)
--- << obj
      ���饤����Ȥ˽��Ϥ��롣
      �ƥ᥽�åɤ�IO��Ʊ̾�Υ᥽�åɤ�Ʊ�ͤ�ư��롣

--- replace(str)
      ���饤����Ȥؤν��Ϥ��ݻ����Ƥ���Хåե������Ƥ�((|str|))��
      �֤������롣

--- cancel
      ���饤����Ȥؤν��Ϥ��ݻ����Ƥ���Хåե������Ƥ��˴����롣

--- escape_html(str)
      &"<>�ʤɤ�ʸ���򥨥������פ��롣

[((<�ܼ������|mod_ruby ���饹��ե����>))]

== Apache::Table

table(Apache�Υǡ�����)���åפ��륯�饹��

=== �����ѡ����饹

Object

=== ���󥯥롼�ɤ��줿���饹

Enumerable

=== �᥽�å�

--- clear
      �ơ��֥�����Ƥ��˴����롣

--- self[name]
--- get(name)
      ((|name|))���б�����ǡ�����������롣

--- self[name]= val
--- set(name, val)
--- setn(name, val)
--- merge(name, val)
--- mergen(name, val)
--- add(name, val)
--- addn(name, val)
      ((|name|))���б�����ǡ��������ꤹ�롣
      �ƥ᥽�åɤΰ㤤�Ϥ褯�狼��ʤ���

--- unset(name)
      ((|name|))���б�����ǡ������������

--- each {|key,val|...}
--- each_key {|key|...}
--- each_value {|val|...}
      �����Ǥ��Ф��ƥ֥��å���¹Ԥ��롣

[((<�ܼ������|mod_ruby ���饹��ե����>))]

=end