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

char eruby_begin_delimiter1	= '<';
char eruby_begin_delimiter2	= '%';
char eruby_end_delimiter1	= '%';
char eruby_end_delimiter2	= '>';
char eruby_expr_char		= '=';
char eruby_comment_char		= '#';

enum embedded_string_type {
    EMBEDDED_STMT,
    EMBEDDED_EXPR,
    EMBEDDED_COMMENT,
};

static int compile_embedded_string(FILE *in, FILE *out,
				   enum embedded_string_type type)
{
    int c, prevc = EOF;

    if (type == EMBEDDED_EXPR)
	fputs("print((", out);
    for (;;) {
	c = getc(in);
    again:
	if (c == eruby_end_delimiter1) {
	    if (prevc == eruby_end_delimiter1)
		continue;
	    c = getc(in);
	    if (c == eruby_end_delimiter2) {
		if (type == EMBEDDED_EXPR)
		    fputs(")); ", out);
		else if (type == EMBEDDED_STMT && prevc != '\n')
		    fputs("; ", out);
		return 0;
	    }
	    else if (c == EOF) {
		if (ferror(in))
		    return SYSTEM_ERROR;
		else
		    return MISSING_END_DELIMITER;
	    }
	    else {
		if (type != EMBEDDED_COMMENT)
		    putc(eruby_end_delimiter1, out);
		prevc = eruby_end_delimiter1;
		goto again;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		if (ferror(in))
		    return SYSTEM_ERROR;
		else
		    return MISSING_END_DELIMITER;
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
    return MISSING_END_DELIMITER;
}

int eruby_compile(FILE *in, FILE *out)
{
    int c, prevc = EOF;
    int err;

    for (;;) {
	c = getc(in);
    again:
	if (c == eruby_begin_delimiter1) {
	    c = getc(in);
	    if (c == eruby_begin_delimiter2) {
		if (prevc != EOF) {
		    fputs("\"; ", out);
		}
		c = getc(in);
		if (c == eruby_begin_delimiter2) {
		    if (prevc == EOF) fputs("print \"", out);
		    putc(eruby_begin_delimiter1, out);
		    putc(eruby_begin_delimiter2, out);
		    prevc = eruby_begin_delimiter2;
		    continue;
		}
		else if (c == eruby_comment_char) {
		    err = compile_embedded_string(in, out, EMBEDDED_COMMENT);
		    if (err) return err;
		}
		else if (c == eruby_expr_char) {
		    err = compile_embedded_string(in, out, EMBEDDED_EXPR);
		    if (err) return err;
		}
		else {
		    if (ungetc(c, in) == EOF)
			return SYSTEM_ERROR;
		    err = compile_embedded_string(in, out, EMBEDDED_STMT);
		    if (err) return err;
		}
		prevc = EOF;
	    } else {
		if (prevc == EOF) fputs("print \"", out);
		putc(eruby_begin_delimiter1, out);
		prevc = eruby_begin_delimiter1;
		goto again;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		goto end;
	    case '\n':
		if (prevc == EOF) fputs("print \"", out);
		fputs("\\n\"\n", out);
		prevc = EOF;
		break;
	    case '\t':
		if (prevc == EOF) fputs("print \"", out);
		fputs("\\t", out);
		prevc = c;
		break;
	    case '\\':
	    case '"':
	    case '#':
		if (prevc == EOF) fputs("print \"", out);
		putc('\\', out);
		putc(c, out);
		prevc = c;
		break;
	    default:
		if (prevc == EOF) fputs("print \"", out);
		putc(c, out);
		prevc = c;
		break;
	    }
	}
    }
 end:
    if (ferror(in))
	return SYSTEM_ERROR;
    if (prevc != EOF) {
	putc('"', out);
    }
    return 0;
}

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
    case MISSING_END_DELIMITER:
	rb_raise(rb_eSyntaxError, "missing end delimiter");
	break;
    case SYSTEM_ERROR:
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

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
