=begin

= mod_ruby ���󥹥ȡ��륬����

[((<�ܼ�|URL:index.ja.jis.html>))
|((<RD����|URL:install.ja.euc.rd>))]

* ((<���󥹥ȡ���>))
* ((<Apache������>))
* ((<eRuby������>))

== ���󥹥ȡ���

�ʲ��μ���mod_ruby�򥤥󥹥ȡ��뤷�ޤ���

  $ tar zxvf mod_ruby-x.y.z.tar.gz
  $ cd mod_ruby-x.y.z/
  $ ./configure.rb --with-apxs=/usr/local/apache/bin/apxs
  $ make
  # make install

== Apache������

�ʲ��Τ褦�������httpd.conf�˵��Ҥ��ޤ���

  LoadModule ruby_module /usr/local/apache/libexec/mod_ruby.so
  
  # ClearModuleList
  # AddModule mod_ruby.c

  <IfModule mod_ruby.c>
    RubyRequire apache/ruby-run
    
    # /ruby�ʲ��Υե������Ruby������ץȤȤ��Ƽ¹Ԥ���
    <Location /ruby>
    SetHandler ruby-object
    RubyHandler Apache::RubyRun.instance
    </Location>
    
    # *.rbx��Ruby������ץȤȤ��Ƽ¹Ԥ���
    <Files *.rbx>
    SetHandler ruby-object
    RubyHandler Apache::RubyRun.instance
    </Files>
  </IfModule>

== eRuby������

eruby�򥤥󥹥ȡ��뤷���ʲ��Τ褦�������httpd.conf�˵��Ҥ��ޤ���

  <IfModule mod_ruby.c>
    RubyRequire apache/eruby-run
    
    # /eruby�ʲ��Υե������eRuby�ե�����Ȥ��ư���
    <Location /eruby>
    SetHandler ruby-object
    RubyHandler Apache::ERubyRun.instance
    </Location>
    
    # *.rhtml��eRuby�ե�����Ȥ��ư�����
    <Files *.rhtml>
    SetHandler ruby-object
    RubyHandler Apache::ERubyRun.instance
    </Files>
  </IfModule>

=end
