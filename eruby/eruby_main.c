/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 * Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>
 *
 * This file is part of eruby.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <ctype.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ruby.h"
#include "re.h"
#include "regex.h"
#include "version.h"

#include "eruby.h"
#include "eruby_logo.h"

EXTERN VALUE ruby_errinfo;
EXTERN VALUE rb_stdout;
EXTERN VALUE rb_defout;
EXTERN VALUE rb_load_path;

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
			     "<div class=\"backtrace\">... %ld levels...\n",
			     ep->len - TRACE_HEAD - TRACE_TAIL);
		else
		    snprintf(buff, BUFSIZ, "         ... %ld levels...<br></div>\n",
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

static void print_generated_code(VALUE code, int cgi)
{
    if (cgi) {
	printf("<tr><th id=\"code\">\n");
	printf("GENERATED CODE\n");
	printf("</th></tr>\n");
	printf("<tr><td headers=\"code\">\n");
	printf("<pre><code>\n");
    }
    else {
	printf("--- generated code ---\n");
    }

    if (cgi) {
	write_escaping_html(RSTRING(code)->ptr, RSTRING(code)->len);
    }
    else {
	fwrite(RSTRING(code)->ptr, 1, RSTRING(code)->len, stdout);
    }
    if (cgi) {
	printf("</code></pre>\n");
	printf("</td></tr>\n");
    }
    else {
	printf("----------------------\n");
    }
}

char rfc822_days[][4] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char rfc822_months[][4] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *rfc1123_time(time_t t)
{
    static char s[32];
    struct tm *tm;

    tm = gmtime(&t);
    sprintf(s, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
	    rfc822_days[tm->tm_wday], tm->tm_mday, rfc822_months[tm->tm_mon],
	    tm->tm_year + 1900,	tm->tm_hour, tm->tm_min, tm->tm_sec);
    return s;
}

static void print_http_headers()
{
    char *tmp;

    if ((tmp = getenv("SERVER_PROTOCOL")) == NULL)
        tmp = "HTTP/1.0";
    printf("%s 200 OK\n", tmp);
    if ((tmp = getenv("SERVER_SOFTWARE")) == NULL)
	tmp = "unknown-server/0.0";
    printf("Server: %s\n", tmp);
    printf("Date: %s\n", rfc1123_time(time(NULL)));
    printf("Connection: close\n");

    return;
}

static void error_print(int state, int mode, VALUE code)
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
	printf("<img src=\"%s/logo.png\" alt=\"eRuby\">\n", imgdir);
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

    if (!NIL_P(code))
	print_generated_code(code, cgi);

    if (cgi) {
        printf("</table>\n");
	printf("</body>\n");
	printf("</html>\n");
    }
}

static VALUE defout_write(VALUE self, VALUE str)
{
    str = rb_obj_as_string(str);
    rb_str_cat(self, RSTRING(str)->ptr, RSTRING(str)->len);
    return Qnil;
}

static VALUE defout_cancel(VALUE self)
{
    if (RSTRING(self)->len == 0) return Qnil;
    RSTRING(self)->len = 0;
    RSTRING(self)->ptr[0] = '\0';
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
    printf("Content-Type: image/png\n\n");
    fwrite(eruby_logo_data, eruby_logo_size, 1, stdout);
}

static void init()
{
    ruby_init();
#if RUBY_VERSION_CODE >= 160
    ruby_init_loadpath();
#else
#if RUBY_VERSION_CODE >= 145
    rb_ary_push(rb_load_path, rb_str_new2("."));
#endif
#endif
    if (eruby_mode == MODE_CGI || eruby_mode == MODE_NPHCGI)
	rb_set_safe_level(1);

    rb_defout = rb_str_new("", 0);
    rb_define_singleton_method(rb_defout, "write", defout_write, 1);
    rb_define_singleton_method(rb_defout, "cancel", defout_cancel, 0);
    eruby_init();
}

static void proc_args(int argc, char **argv)
{
    switch (eruby_parse_options(argc, argv)) {
    case 1:
	exit(0);
    case 2:
	exit(2);
    }

    if (eruby_mode == MODE_UNKNOWN)
	eruby_mode = guess_mode();

    if (eruby_mode == MODE_CGI || eruby_mode == MODE_NPHCGI) {
	char *path;
	char *script_filename;
	char *path_translated;

	if ((path = getenv("PATH_INFO")) != NULL &&
	    strcmp(path, "/logo.png") == 0) {
	    give_img_logo(eruby_mode);
	    exit(0);
	}
	
	if ((script_filename = getenv("SCRIPT_FILENAME")) == NULL
	    || strstr(script_filename, argv[0]) != NULL)
	    eruby_filename = NULL;
	if ((path_translated = getenv("PATH_TRANSLATED")) != NULL) {
	    eruby_filename = path_translated;
	}
	if (eruby_filename == NULL)
	    eruby_filename = "";
    }
    else {
	if (eruby_filename == NULL)
	    eruby_filename = "-";
    }
}

static void run()
{
    VALUE code;
    int state;
    char *out;
    int nout;

    code = eruby_load(eruby_filename, 0, &state);
    if (state && !rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	error_print(state, eruby_mode, code);
	exit(0);
    }
    if (eruby_mode == MODE_FILTER && (RTEST(ruby_debug) || RTEST(ruby_verbose))) {
	print_generated_code(code, 0);
    }
    out = RSTRING(rb_defout)->ptr;
    nout = RSTRING(rb_defout)->len;
    if (!eruby_noheader &&
	(eruby_mode == MODE_CGI || eruby_mode == MODE_NPHCGI)) {
	if (eruby_mode == MODE_NPHCGI)
	    print_http_headers();

	printf("Content-Type: text/html; charset=%s\n", ERUBY_CHARSET);
	printf("Content-Length: %d\n", nout);
	printf("\n");
    }
    fwrite(out, nout, 1, stdout);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    init();
    proc_args(argc, argv);
    run();
    return 0;
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
