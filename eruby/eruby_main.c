/*
 * $Id$
 * Copyright (C) 1999  Network Applied Communication Laboratory, Inc.
 */

#include "ruby.h"
#include "re.h"
#include "regex.h"
#include "version.h"

#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eruby.h"
#include "eruby_logo.h"

extern VALUE ruby_errinfo;
extern VALUE rb_stdout;
extern VALUE rb_defout;

/* copied from eval.c */
#define TAG_RETURN	0x1
#define TAG_BREAK	0x2
#define TAG_NEXT	0x3
#define TAG_RETRY	0x4
#define TAG_REDO	0x5
#define TAG_RAISE	0x6
#define TAG_THROW	0x7
#define TAG_FATAL	0x8
#define TAG_MASK	0xf

#define MODE_UNKNOWN    1
#define MODE_FILTER     2
#define MODE_CGI        4
#define MODE_NPHCGI     8

static char *get_charset()
{
    switch (rb_kcode()) {
    case MBCTYPE_EUC:
	return "EUC-JP";
    case MBCTYPE_SJIS:
	return "SHIFT_JIS";
    case MBCTYPE_UTF8:
	return "UTF8";
    case MBCTYPE_ASCII:
    default:
	return "US-ASCII";
    }
}

static void write_escaping_html(char *str, int len)
{
    int i;
    for (i = 0; i < len; i++) {
	switch (str[i]) {
	case '&':
	    fputs("&amp;", stdout);
	    break;
	case '<':
	    fputs("&lt;", stdout);
	    break;
	case '>':
	    fputs("&gt;", stdout);
	    break;
	case '"':
	    fputs("&quot;", stdout);
	    break;
	default:
	    putc(str[i], stdout);
	    break;
	}
    }
}

static void error_pos(int cgi)
{
    char buff[BUFSIZ];
    ID last_func = rb_frame_last_func();

    if (ruby_sourcefile) {
	if (last_func) {
	    snprintf(buff, BUFSIZ, "%s:%d:in `%s'", ruby_sourcefile, ruby_sourceline,
		     rb_id2name(last_func));
	}
	else {
	    snprintf(buff, BUFSIZ, "%s:%d", ruby_sourcefile, ruby_sourceline);
	}
	if (cgi)
	    write_escaping_html(buff, strlen(buff));
	else
	    fputs(buff, stdout);
    }
}

static void exception_print(int cgi)
{
    VALUE errat;
    VALUE eclass;
    VALUE einfo;

    if (NIL_P(ruby_errinfo)) return;

    errat = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    if (!NIL_P(errat)) {
	VALUE mesg = RARRAY(errat)->ptr[0];

	if (NIL_P(mesg)) {
	    error_pos(cgi);
	}
	else {
	    if (cgi)
		write_escaping_html(RSTRING(mesg)->ptr, RSTRING(mesg)->len);
	    else
		fwrite(RSTRING(mesg)->ptr, 1, RSTRING(mesg)->len, stdout);
	}
    }

    eclass = CLASS_OF(ruby_errinfo);
    einfo = rb_obj_as_string(ruby_errinfo);
    if (eclass == rb_eRuntimeError && RSTRING(einfo)->len == 0) {
	printf(": unhandled exception\n");
    }
    else {
	VALUE epath;

	epath = rb_class_path(eclass);
	if (RSTRING(einfo)->len == 0) {
	    printf(": ");
	    if (cgi)
		write_escaping_html(RSTRING(epath)->ptr, RSTRING(epath)->len);
	    else
		fwrite(RSTRING(epath)->ptr, 1, RSTRING(epath)->len, stdout);
	    if (cgi)
		printf("<br>\n");
	    else
		printf("\n");
	}
	else {
	    char *tail  = 0;
	    int len = RSTRING(einfo)->len;

	    if (RSTRING(epath)->ptr[0] == '#') epath = 0;
	    if ((tail = strchr(RSTRING(einfo)->ptr, '\n')) != NULL) {
		len = tail - RSTRING(einfo)->ptr;
		tail++;		/* skip newline */
	    }
	    printf(": ");
	    if (cgi)
		write_escaping_html(RSTRING(einfo)->ptr, len);
	    else
		fwrite(RSTRING(einfo)->ptr, 1, len, stdout);
	    if (epath) {
		printf(" (");
		if (cgi)
		    write_escaping_html(RSTRING(epath)->ptr, RSTRING(epath)->len);
		else
		    fwrite(RSTRING(epath)->ptr, 1, RSTRING(epath)->len, stdout);
		if (cgi)
		    printf(")<br>\n");
		else
		    printf(")\n");
	    }
	    if (tail) {
		if (cgi)
		    write_escaping_html(tail, RSTRING(einfo)->len - len - 1);
		else
		    fwrite(tail, 1, RSTRING(einfo)->len - len - 1, stdout);
		if (cgi)
		    printf("<br>\n");
		else
		    printf("\n");
	    }
	}
    }

    if (!NIL_P(errat)) {
	int i;
	struct RArray *ep = RARRAY(errat);

#define TRACE_MAX (TRACE_HEAD+TRACE_TAIL+5)
#define TRACE_HEAD 8
#define TRACE_TAIL 5

	rb_ary_pop(errat);
	ep = RARRAY(errat);
	for (i=1; i<ep->len; i++) {
	    if (TYPE(ep->ptr[i]) == T_STRING) {
		if (cgi) {
		    printf("<div class=\"backtrace\">from ");
		    write_escaping_html(RSTRING(ep->ptr[i])->ptr,
					RSTRING(ep->ptr[i])->len);
		}
		else {
		    printf("        from ");
		    fwrite(RSTRING(ep->ptr[i])->ptr, 1,
			   RSTRING(ep->ptr[i])->len, stdout);
		}
		if (cgi)
		    printf("<br></div>\n");
		else
		    printf("\n");
	    }
	    if (i == TRACE_HEAD && ep->len > TRACE_MAX) {
		char buff[BUFSIZ];
		if (cgi)
		    snprintf(buff, BUFSIZ,
			     "<div class=\"backtrace\">... %d levels...\n",
			     ep->len - TRACE_HEAD - TRACE_TAIL);
		else
		    snprintf(buff, BUFSIZ, "         ... %d levels...<br></div>\n",
			     ep->len - TRACE_HEAD - TRACE_TAIL);
		if (cgi)
		    write_escaping_html(buff, strlen(buff));
		else
		    fputs(buff, stdout);
		i = ep->len - TRACE_TAIL;
	    }
	}
    }
}

static int is_errline(int lineno, int errc, int *errv)
{
    int i;

    if (errc == 0 || lineno < errv[0] || errv[errc - 1] < lineno)
	return 0;
    for (i = 0; i < errc; i++) {
	if (lineno == errv[i])
	    return 1;
    }
    return 0;
}

static void print_generated_code(VALUE script, int cgi)
{
    FILE *f;
    char buff[BUFSIZ];
    int len = 1;
    int lineno = 1;
    int print_lineno = 1;
    int errc = 0;
    int *errv = NULL;
    int errline = 0;

    if ((f = fopen(RSTRING(script)->ptr, "r")) == NULL)
	return;
    if (cgi) {
	printf("<tr><th id=\"code\">\n");
	printf("GENERATED CODE\n");
	printf("</th></tr>\n");
	printf("<tr><td headers=\"code\">\n");
	printf("<pre><code>\n");
	if (!NIL_P(ruby_errinfo)) {
	    VALUE errat = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
	    int i, n;
	    char fmt[BUFSIZ];

	    if (!NIL_P(errat)) {
		errv = ALLOCA_N(int, RARRAY(errat)->len);
		snprintf(fmt, BUFSIZ, "%s:%%d", RSTRING(script)->ptr);
		for (i = 0; i < RARRAY(errat)->len; i++) {
		    if (sscanf(RSTRING(RARRAY(errat)->ptr[i])->ptr, fmt, &n) == 1) {
			errv[errc++] = n;
		    }
		}
	    }
	}
    }
    else {
	printf("--- generated code ---\n");
    }

    while (fgets(buff, BUFSIZ, f) != NULL) {
	if (print_lineno) {
	    if (cgi && is_errline(lineno, errc, errv)) {
		printf("<strong>");
		errline = 1;
	    }
	    printf("%5d: ", lineno++);
	}
	len = strlen(buff);
	print_lineno = buff[len - 1] == '\n';
	if (cgi) {
	    if (print_lineno && errline) buff[--len] = '\0';
	    write_escaping_html(buff, len);
	    if (print_lineno && errline) {
		printf("</strong>\n");
		errline = 0;
	    }
	}
	else {
	    fwrite(buff, 1, len, stdout);
	}
    }

    if (cgi) {
	printf("</code></pre>\n");
	printf("</td></tr>\n");
    }
    else {
	printf("----------------------\n");
    }
}

static void print_http_headers()
{
    char *tmp;
    time_t t;
    struct tm *tm;

    if ((tmp = getenv("SERVER_PROTOCOL")) == NULL)
        tmp = "HTTP/1.0";
    printf("%s 200 OK\n", tmp);
    if ((tmp = getenv("SERVER_SOFTWARE")) == NULL)
	tmp = "unknown-server/0.0";
    printf("Server: %s\n", tmp);
    t = time(&t);
    tm = localtime(&t);
    tmp = ctime(&t);
    tmp[strlen(tmp)-1] = '\0';
    printf("Date: %s\n", tmp);
    printf("Connection: close\n");

    return;
}

static void error_print(int state, int mode, VALUE script)
{
    char buff[BUFSIZ];
    int cgi = mode == MODE_CGI || mode == MODE_NPHCGI;

    rb_defout = rb_stdout;
    if (cgi) {
	char *imgdir;
	if ((imgdir = getenv("SCRIPT_NAME")) == NULL)
	    imgdir = "UNKNOWN_IMG_DIR";
        if (mode == MODE_NPHCGI)
            print_http_headers();
        printf("Content-Type: text/html\n");
        printf("Content-Style-Type: text/css\n");
        printf("\n");
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<title>eRuby</title>\n");
	printf("<style type=\"text/css\">\n");
	printf("<!--\n");
	printf("body { background-color: #ffffff }\n");
	printf("table { width: 100%%; padding: 5pt; border-style: none }\n");
	printf("th { color: #6666ff; background-color: #b0d0d0; text-align: left }\n");
	printf("td { color: #336666; background-color: #d0ffff }\n");
	printf("strong { color: #ff0000; font-weight: bold }\n");
	printf("div.backtrace { text-indent: 15%% }\n");
	printf("#version { color: #ff9900 }\n");
	printf("-->\n");
	printf("</style>\n");
	printf("</head>\n");
	printf("<body>\n");
        printf("<table summary=\"eRuby error information\">\n");
        printf("<caption>\n");
	printf("<img src=\"%s/logo.gif\" alt=\"eRuby\">\n", imgdir);
        printf("<span id=version>version: %s</span>\n", ERUBY_VERSION);
        printf("</caption>\n");
        printf("<tr><th id=\"error\">\n");
        printf("ERROR\n");
        printf("</th></tr>\n");
        printf("<tr><td headers=\"error\">\n");
    }

    switch (state) {
    case TAG_RETURN:
	error_pos(cgi);
	printf(": unexpected return\n");
	break;
    case TAG_NEXT:
	error_pos(cgi);
	printf(": unexpected next\n");
	break;
    case TAG_BREAK:
	error_pos(cgi);
	printf(": unexpected break\n");
	break;
    case TAG_REDO:
	error_pos(cgi);
	printf(": unexpected redo\n");
	break;
    case TAG_RETRY:
	error_pos(cgi);
	printf(": retry outside of rescue clause\n");
	break;
    case TAG_RAISE:
    case TAG_FATAL:
	exception_print(cgi);
	break;
    default:
	error_pos(cgi);
	snprintf(buff, BUFSIZ, ": unknown longjmp status %d", state);
	fputs(buff, stdout);
	break;
    }
    if (cgi) {
        printf("</td></tr>\n");
    }

    if (!NIL_P(script))
	print_generated_code(script, cgi);

    if (cgi) {
        printf("</table>\n");
	printf("</body>\n");
	printf("</html>\n");
    }
}

static VALUE str_write(VALUE self, VALUE str)
{
    str = rb_obj_as_string(str);
    rb_str_cat(self, RSTRING(str)->ptr, RSTRING(str)->len);
    return Qnil;
}

static int guess_mode()
{
    if (getenv("GATEWAY_INTERFACE") == NULL) {
	return MODE_FILTER;
    }
    else {
	char *name = getenv("SCRIPT_FILENAME");
	char buff[BUFSIZ];
	
	if (name != NULL) {
	    strcpy(buff, name);
	    if ((name = strrchr(buff, '/')) != NULL) 
		*name++ = '\0';
	    else 
		name = buff;
	    if (strncasecmp(name, "nph-", 4) == 0) 
		return MODE_NPHCGI;
	    else
		return MODE_CGI;
	}
	else {
	    return MODE_CGI;
	}
    }
}

static void give_img_logo(int mode)
{
    if (mode == MODE_NPHCGI)
	print_http_headers();
    printf("Content-Type: image/gif\n\n");
    fwrite(eruby_logo_data, eruby_logo_size, 1, stdout);
}

static char *parse_options(int argc, char **argv)
{
    char *filename = NULL;
    int i;

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') {
	    char *opt = argv[i] + 1;

	    if (*opt == '\0') {
		if (filename == NULL)
		    filename = argv[i];
	    }
	    else if (strcmp(opt, "d") == 0) {
		ruby_debug = 1;
	    }
	    else if (strcmp(opt, "v") == 0) {
		ruby_verbose = 1;
	    }
	    else {
		fprintf(stderr, "usage: %s [-dv] [file]\n", argv[0]);
		exit(1);
	    }
	}
	else {
	    if (filename == NULL)
		filename = argv[i];
	}
    }
    return filename;
}

int main(int argc, char **argv)
{
    char *path;
    char *filename = NULL;
    VALUE script;
    int state;
    int mode = MODE_UNKNOWN;
    char *out;
    int nout;

    if (mode == MODE_UNKNOWN)
	mode = guess_mode();

    if (mode == MODE_CGI || mode == MODE_NPHCGI) {
	if ((path = getenv("PATH_INFO")) != NULL &&
	    strcmp(path, "/logo.gif") == 0) {
	    give_img_logo(mode);
	    return 0;
	}
	if ((filename = getenv("PATH_TRANSLATED")) == NULL)
	    filename = "";
    }
    else {
	if ((filename = parse_options(argc, argv)) == NULL)
	    filename = "-";
    }

    ruby_init();
    if (mode == MODE_CGI || mode == MODE_NPHCGI)
	rb_set_safe_level(1);

    rb_defout = rb_str_new("", 0);
    rb_define_singleton_method(rb_defout, "write", str_write, 1);
    script = eruby_load(filename, 0, &state);
    if (state) {
	error_print(state, mode, script);
	if (!NIL_P(script))
	    unlink(RSTRING(script)->ptr);
	return 0;
    }
    if (mode == MODE_FILTER && (ruby_debug || ruby_verbose)) {
	print_generated_code(script, 0);
    }
    unlink(RSTRING(script)->ptr);
    out = RSTRING(rb_defout)->ptr;
    nout = RSTRING(rb_defout)->len;
    if (mode == MODE_CGI || mode == MODE_NPHCGI) {
	if (mode == MODE_NPHCGI)
	    print_http_headers();

	printf("Content-Type: text/html; charset=%s\n", get_charset());
	printf("Content-Length: %d\n", nout);
	printf("\n");
    }
    fwrite(out, nout, 1, stdout);
    fflush(stdout);
    return 0;
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
