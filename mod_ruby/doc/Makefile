EN_RDS		= index.en.rd \
		  install.en.rd \
		  directives.en.rd \
		  classes.en.rd \
		  faq.en.rd
JA_RDS		= $(EN_RDS:.en.rd=.ja.euc.rd)
RDS		= $(EN_RDS) $(JA_RDS)
EN_HTMLS	= $(EN_RDS:.en.rd=.en.html)
JA_HTMLS	= $(JA_RDS:.ja.euc.rd=.ja.jis.html)
HTMLS		= $(EN_HTMLS) $(JA_HTMLS)

.SUFFIXES: .en.rd .ja.euc.rd .en.html .ja.jis.html

.en.rd.en.html:
	ruby -Kn -S rd2 -r rd/rd2html-lib \
		--with-css=default.css \
		$< > $@

.ja.euc.rd.ja.jis.html:
	ruby -Ke -S rd2 -r rd/rd2html-lib \
		--with-css=default.css --out-code=jis \
		$< > $@

all: $(HTMLS)

clean:
	rm -f $(HTMLS)
	rm -f *~
