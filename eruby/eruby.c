/*
 * $Id$
 * Copyright (C) 1999  Network Applied Communication Laboratory, Inc.
 */

#include "ruby.h"

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eruby.h"
#include "config.h"

static VALUE mERuby;
int eruby_noheader = 0;
VALUE eruby_charset;

static char eruby_begin_delimiter[] = "<%";
static char eruby_end_delimiter[] = "%>";
static char eruby_expr_char = '=';
static char eruby_comment_char = '#';
static char eruby_line_beg_char = '%';

enum embedded_program_type {
    EMBEDDED_STMT,
    EMBEDDED_EXPR,
    EMBEDDED_COMMENT,
};

#define EOP (-2)

static int parse_embedded_program(FILE *in, FILE *out,
				  enum embedded_program_type type)
{
    int c, prevc = EOF;

    if (type == EMBEDDED_EXPR)
	fputs("print((", out);
    for (;;) {
	c = getc(in);
    again:
	if (c == eruby_end_delimiter[0]) {
	    c = getc(in);
	    if (c == eruby_end_delimiter[1]) {
		if (prevc == eruby_end_delimiter[0]) {
		    if (type != EMBEDDED_COMMENT)
			putc(eruby_end_delimiter[1], out);
		    prevc = eruby_end_delimiter[1];
		    continue;
		}
		if (type == EMBEDDED_EXPR)
		    fputs(")); ", out);
		else if (type == EMBEDDED_STMT && prevc != '\n')
		    fputs("; ", out);
		return ERUBY_OK;
	    }
	    else if (c == EOF) {
		if (ferror(in))
		    return ERUBY_SYSTEM_ERROR;
		else
		    return ERUBY_MISSING_END_DELIMITER;
	    }
	    else {
		if (type != EMBEDDED_COMMENT)
		    putc(eruby_end_delimiter[0], out);
		prevc = eruby_end_delimiter[0];
		goto again;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		if (ferror(in))
		    return ERUBY_SYSTEM_ERROR;
		else
		    return ERUBY_MISSING_END_DELIMITER;
	    case '\n':
		putc(c, out);
		prevc = c;
		break;
	    default:
		if (type != EMBEDDED_COMMENT)
		    putc(c, out);
		prevc = c;
		break;
	    }
	}
    }
    return ERUBY_MISSING_END_DELIMITER;
}

static int parse_embedded_line(FILE *in, FILE *out)
{
    int c;

    for (;;) {
	c = getc(in);
	switch (c) {
	case EOF:
	    if (ferror(in))
		return ERUBY_SYSTEM_ERROR;
	    else
		return ERUBY_MISSING_END_DELIMITER;
	case '\n':
	    putc(c, out);
	    return ERUBY_OK;
	    break;
	default:
	    putc(c, out);
	    break;
	}
    }
    return ERUBY_MISSING_END_DELIMITER;
}

int eruby_compile(FILE *in, FILE *out)
{
    int c, prevc = EOF;
    int err;

    c = getc(in);
    if (c == '#') {
	do {
	    c = getc(in);
	    if (c == '\n') {
		putc('\n', out);
		break;
	    }
	} while (c != EOF);
    }
    else {
	ungetc(c, in);
    }

    for (;;) {
	c = getc(in);
    again:
	if (c == eruby_begin_delimiter[0]) {
	    c = getc(in);
	    if (c == eruby_begin_delimiter[1]) {
		c = getc(in);
		if (c == EOF) {
		    return ERUBY_MISSING_END_DELIMITER;
		}
		else if (c == eruby_begin_delimiter[1]) { /* <%% => <% */
		    if (prevc < 0) fputs("print \"", out);
		    fwrite(eruby_begin_delimiter, 1, 2, out);
		    prevc = eruby_begin_delimiter[1];
		    continue;
		}
		else {
		    if (prevc >= 0)
			fputs("\"; ", out);
		    if (c == eruby_comment_char) {
			err = parse_embedded_program(in, out, EMBEDDED_COMMENT);
		    }
		    else if (c == eruby_expr_char) {
			err = parse_embedded_program(in, out, EMBEDDED_EXPR);
		    }
		    else {
			if (ungetc(c, in) == EOF)
			    return ERUBY_SYSTEM_ERROR;
			err = parse_embedded_program(in, out, EMBEDDED_STMT);
		    }
		    if (err) return err;
		    prevc = EOP;
		}
	    } else {
		if (prevc < 0) fputs("print \"", out);
		putc(eruby_begin_delimiter[0], out);
		prevc = eruby_begin_delimiter[0];
		goto again;
	    }
	}
	else if (c == eruby_line_beg_char && prevc == EOF) {
	    c = getc(in);
	    if (c == EOF) {
		return ERUBY_MISSING_END_DELIMITER;
	    }
	    else if (c == eruby_line_beg_char) { /* %% => % */
		if (prevc < 0) fputs("print \"", out);
		fputc(c, out);
		prevc = c;
	    }
	    else {
		if (ungetc(c, in) == EOF)
		    return ERUBY_SYSTEM_ERROR;
		err = parse_embedded_line(in, out);
		if (err) return err;
		prevc = EOF;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		goto end;
	    case '\n':
		if (prevc < 0) fputs("print \"", out);
		fputs("\\n\"\n", out);
		prevc = EOF;
		break;
	    case '\t':
		if (prevc < 0) fputs("print \"", out);
		fputs("\\t", out);
		prevc = c;
		break;
	    case '\\':
	    case '"':
	    case '#':
		if (prevc < 0) fputs("print \"", out);
		putc('\\', out);
		putc(c, out);
		prevc = c;
		break;
	    default:
		if (prevc < 0) fputs("print \"", out);
		putc(c, out);
		prevc = c;
		break;
	    }
	}
    }
 end:
    if (ferror(in))
	return ERUBY_SYSTEM_ERROR;
    if (prevc != EOF) {
	putc('"', out);
    }
    return ERUBY_OK;
}

#ifndef S_ISDIR
# define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
#endif

#ifndef W_OK
# define W_OK 2
#endif

static char *check_dir(char *dir)
{
    struct stat st;

    if (dir == NULL) return NULL;
    if (stat(dir, &st) < 0) return NULL;
    if (!S_ISDIR(st.st_mode)) return NULL;
    if (eaccess(dir, W_OK) < 0) return NULL;
    return dir;
}

static char *eruby_mktemp(char *file)
{
    char *dir;
    char *tmp;
    char *buf;

    dir = check_dir(getenv("TMP"));
    if (!dir) dir = check_dir(getenv("TMPDIR"));
    if (!dir) dir = "/tmp";

    if ((tmp = strrchr(file, '/')) != NULL)
	file = tmp + 1;

    buf = ALLOC_N(char, strlen(dir) + strlen(file) + 10);
    sprintf(buf, "%s/%s.XXXXXX", dir, file);
    dir = mktemp(buf);
    if (dir == NULL) free(buf);

    return dir;
}

VALUE eruby_compile_file(char *filename)
{
    char *tmp;
    VALUE scriptname;
    FILE *in, *out;
    int err;

    if (strcmp(filename, "-") == 0) {
	in = stdin;
    }
    else {
	if ((in = fopen(filename, "r")) == NULL)
	    rb_sys_fail(filename);
    }
    if ((tmp = eruby_mktemp(filename)) == NULL)
	rb_fatal("Can't mktemp");
    scriptname = rb_str_new2(tmp);
    free(tmp);
    if ((out = fopen(RSTRING(scriptname)->ptr, "w")) == NULL)
	rb_fatal("Cannot open temporary file: %s", RSTRING(scriptname)->ptr);
    err = eruby_compile(in, out);
    if (in != stdin) fclose(in);
    fclose(out);
    switch (err) {
    case ERUBY_MISSING_END_DELIMITER:
	rb_raise(rb_eSyntaxError, "missing end delimiter");
	break;
    case ERUBY_SYSTEM_ERROR:
	rb_sys_fail(filename);
	break;
    default:
	return scriptname;
    }
}

VALUE eruby_load(char *filename, int wrap, int *state)
{
    VALUE scriptname;

    scriptname = rb_protect(eruby_compile_file, (VALUE) filename, state);
    if (*state)	return Qnil;
    rb_load_protect(scriptname, wrap, state);
    return scriptname;
}

static VALUE noheader_getter()
{
    return eruby_noheader ? Qtrue : Qfalse;
}

static void noheader_setter(VALUE val)
{
    eruby_noheader = RTEST(val);
}

static VALUE eruby_get_noheader(VALUE self)
{
    return eruby_noheader ? Qtrue : Qfalse;
}

static VALUE eruby_set_noheader(VALUE self, VALUE val)
{
    eruby_noheader = RTEST(val);
    return val;
}

static VALUE eruby_get_charset(VALUE self)
{
    return eruby_charset;
}

static VALUE eruby_set_charset(VALUE self, VALUE val)
{
    Check_Type(val, T_STRING);
    eruby_charset = val;
    return val;
}

void eruby_init()
{
    rb_define_virtual_variable("$NOHEADER", noheader_getter, noheader_setter);

    mERuby = rb_define_module("ERuby");
    rb_define_module_function(mERuby, "noheader", eruby_get_noheader, 0);
    rb_define_module_function(mERuby, "noheader=", eruby_set_noheader, 1);
    rb_define_module_function(mERuby, "charset", eruby_get_charset, 0);
    rb_define_module_function(mERuby, "charset=", eruby_set_charset, 1);

    eruby_charset = rb_str_new2(ERUBY_DEFAULT_CHARSET);
    rb_global_variable(&eruby_charset);
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
