/*
 * $Id$
 * Copyright (C) 1999  Network Applied Communication Laboratory, Inc.
 */

#include "ruby.h"
#include "rubysig.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

#include "eruby.h"
#include "config.h"

static VALUE mERuby;

char *eruby_filename = NULL;
int eruby_mode = MODE_UNKNOWN;
int eruby_noheader = 0;
VALUE eruby_charset;
VALUE eruby_default_charset;

#define ERUBY_BEGIN_DELIMITER "<%"
#define ERUBY_END_DELIMITER "%>"
#define ERUBY_EXPR_CHAR '='
#define ERUBY_COMMENT_CHAR '#'
#define ERUBY_LINE_BEG_CHAR '%'

enum embedded_program_type {
    EMBEDDED_STMT,
    EMBEDDED_EXPR,
    EMBEDDED_COMMENT,
};

#define EOP (-2)

#if defined(_MSC_VER)
#define SIGQUIT 3
#endif

static char *readline(FILE *f)
{
    char buff[BUFSIZ];
    char *line;
    int buff_len, line_len = 0;

    line = (char *) malloc(BUFSIZ);
    if (line == NULL)
	return NULL;
    for (;;) {
	if (fgets(buff, BUFSIZ, f) == NULL) {
	    if (ferror(f)) {
		free(line);
		return NULL;
	    }
	    return line;
	}
	strcpy(line + line_len, buff);
	buff_len = strlen(buff);
	line_len += buff_len;
	if (buff[buff_len - 1] == '\n' || buff_len < BUFSIZ - 1) {
	    break;
	}
	else {
	    char *tmp;
	    tmp = (char *) realloc(line, line_len + BUFSIZ);
	    if (tmp == NULL) {
		free(line);
		return NULL;
	    }
	    line = tmp;
	}
    }
    return line;
}

static void usage(char *progname)
{
    fprintf(stderr, "\
usage: %s [switches] [inputfile]\n\n\
  -d, --debug		set debugging flags (set $DEBUG to true)\n\
  -K[kcode]		specifies KANJI (Japanese) code-set\n\
  -M[mode]		specifies runtime mode\n\
			  f: filter mode\n\
			  c: CGI mode\n\
			  n: NPH-CGI mode\n\
  -C [charset]		specifies charset parameter for Content-Type\n\
  -n, --noheader	disables CGI header output\n\
  -v, --verbose		enables verbose mode\n\
  --version		print version information and exit\n\
\n", progname);
}

static void show_version()
{
    fprintf(stderr, "eRuby version %s\n", ERUBY_VERSION);
    ruby_show_version();
}

static int set_mode(char *mode)
{
    switch (*mode) {
    case 'f':
	eruby_mode = MODE_FILTER;
	break;
    case 'c':
	eruby_mode = MODE_CGI;
	break;
    case 'n':
	eruby_mode = MODE_NPHCGI;
	break;
    default:
	fprintf(stderr, "invalid mode -- %s\n", mode);
	return -1;
    }
    return 0;
}

int eruby_parse_options(int argc, char **argv)
{
    unsigned char *s;
    int i;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-' || argv[i][1] == '\0') {
	    eruby_filename = argv[i];
	    break;
	}
	s = argv[i];
      again:
	while (isspace(*s))
	    s++;
	if (*s == '-') s++;
	switch (*s) {
	case 'M':
	    if (set_mode(++s) == -1)
		return 2;
	    s++;
	    goto again;
	case 'K':
	    s++;
	    if (*s == '\0') {
		fprintf(stderr, "%s: no arg given for -K\n", argv[0]);
		return 2;
	    }
	    rb_set_kcode(s);
	    s++;
	    goto again;
	case 'C':
	    s++;
	    if (isspace(*s)) s++;
	    if (*s == '\0') {
		i++;
		if (i == argc) {
		    fprintf(stderr, "%s: no arg given for -C\n", argv[0]);
		    return 2;
		}
		eruby_charset = rb_str_new2(argv[i]);
		break;
	    }
	    else {
		unsigned char *p = s;
		while (*p && !isspace(*p)) p++;
		eruby_charset = rb_str_new(s, p - s);
		s = p;
		goto again;
	    }
	case 'd':
	    ruby_debug = Qtrue;
	    s++;
	    goto again;
	case 'v':
	    ruby_verbose = Qtrue;
	    s++;
	    goto again;
	case 'n':
	    eruby_noheader = 1;
	    s++;
	    goto again;
	case '\0':
	    break;
	case 'h':
	    usage(argv[0]);
	    return 1;
	case '-':
	    s++;
	    if (strncmp(s , "debug", 5) == 0
		&& (s[5] == '\0' || isspace(s[5]))) {
		ruby_debug = Qtrue;
		s += 5;
		goto again;
	    }
	    else if (strncmp(s, "noheader", 8) == 0
		     && (s[8] == '\0' || isspace(s[8]))) {
		eruby_noheader = 1;
		s += 8;
		goto again;
	    }
	    else if (strncmp(s, "version", 7) == 0
		     && (s[7] == '\0' || isspace(s[7]))) {
		show_version();
		return 1;
	    }
	    else if (strncmp(s, "verbose", 7) == 0
		     && (s[7] == '\0' || isspace(s[7]))) {
		ruby_verbose = Qtrue;
		s += 7;
		goto again;
	    }
	    else if (strncmp(s, "help", 4) == 0
		     && (s[4] == '\0' || isspace(s[4]))) {
		usage(argv[0]);
		return 1;
	    }
	    else {
		fprintf(stderr, "%s: invalid option -- %s\n", argv[0], s);
		fprintf(stderr, "try `%s --help' for more information.\n", argv[0]);
		return 1;
	    }
	default:
	    fprintf(stderr, "%s: invalid option -- %s\n", argv[0], s);
	    fprintf(stderr, "try `%s --help' for more information.\n", argv[0]);
	    return 2;
	}
    }

    return 0;
}

static int parse_embedded_program(FILE *in, FILE *out,
				  enum embedded_program_type type)
{
    int c, prevc = EOF;

    if (type == EMBEDDED_EXPR)
	fputs("print((", out);
    for (;;) {
	TRAP_BEG;
	c = getc(in);
	TRAP_END;
      again:
	if (c == ERUBY_END_DELIMITER[0]) {
	    TRAP_BEG;
	    c = getc(in);
	    TRAP_END;
	    if (c == ERUBY_END_DELIMITER[1]) {
		if (prevc == ERUBY_END_DELIMITER[0]) {
		    if (type != EMBEDDED_COMMENT)
			putc(ERUBY_END_DELIMITER[1], out);
		    prevc = ERUBY_END_DELIMITER[1];
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
		    putc(ERUBY_END_DELIMITER[0], out);
		prevc = ERUBY_END_DELIMITER[0];
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
	TRAP_BEG;
	c = getc(in);
	TRAP_END;
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

    TRAP_BEG;
    c = getc(in);
    TRAP_END;
    if (c == '#') {
	TRAP_BEG;
	c = getc(in);
	TRAP_END;
	if (c == '!') {
	    unsigned char *p;
	    char *argv[2];
	    char *line = readline(in);

	    if (line == NULL)
		return ERUBY_SYSTEM_ERROR;
	    if (line[strlen(line) - 1] == '\n') {
		line[strlen(line) - 1] = '\0';
		putc('\n', out);
	    }
	    argv[0] = "eruby";
	    p = line;
	    while (isspace(*p)) p++;
	    while (*p && !isspace(*p)) p++;
	    while (isspace(*p)) p++;
	    argv[1] = p;
	    if (eruby_parse_options(2, argv) != 0) {
		free(line);
		return ERUBY_INVALID_OPTION;
	    }
	    free(line);
	}
	else {
	    while (c != EOF) {
		TRAP_BEG;
		c = getc(in);
		TRAP_END;
		if (c == '\n') {
		    putc(c, out);
		    break;
		}
	    }
	}
    }
    else {
	ungetc(c, in);
    }

    for (;;) {
	TRAP_BEG;
	c = getc(in);
	TRAP_END;
      again:
	if (c == ERUBY_BEGIN_DELIMITER[0]) {
	    TRAP_BEG;
	    c = getc(in);
	    TRAP_END;
	    if (c == ERUBY_BEGIN_DELIMITER[1]) {
		TRAP_BEG;
		c = getc(in);
		TRAP_END;
		if (c == EOF) {
		    return ERUBY_MISSING_END_DELIMITER;
		}
		else if (c == ERUBY_BEGIN_DELIMITER[1]) { /* <%% => <% */
		    if (prevc < 0) fputs("print \"", out);
		    fwrite(ERUBY_BEGIN_DELIMITER, 1, 2, out);
		    prevc = ERUBY_BEGIN_DELIMITER[1];
		    continue;
		}
		else {
		    if (prevc >= 0)
			fputs("\"; ", out);
		    if (c == ERUBY_COMMENT_CHAR) {
			err = parse_embedded_program(in, out, EMBEDDED_COMMENT);
		    }
		    else if (c == ERUBY_EXPR_CHAR) {
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
		putc(ERUBY_BEGIN_DELIMITER[0], out);
		prevc = ERUBY_BEGIN_DELIMITER[0];
		goto again;
	    }
	}
	else if (c == ERUBY_LINE_BEG_CHAR && prevc == EOF) {
	    TRAP_BEG;
	    c = getc(in);
	    TRAP_END;
	    if (c == EOF) {
		return ERUBY_MISSING_END_DELIMITER;
	    }
	    else if (c == ERUBY_LINE_BEG_CHAR) { /* %% => % */
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
    int fd;
    int err;

    if (strcmp(filename, "-") == 0) {
	in = stdin;
    }
    else {
	if ((in = fopen(filename, "r")) == NULL)
	    rb_sys_fail(filename);
    }
  retry:
    if ((tmp = eruby_mktemp(filename)) == NULL) {
	fclose(in);
	rb_fatal("can't mktemp");
    }
    fd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 00600);
    if (fd < 0) {
	free(tmp);
	if (errno == EEXIST)
	    goto retry;
	fclose(in);
	rb_fatal("cannot open temporary file: %s", tmp);
    }
    scriptname = rb_str_new2(tmp);
    free(tmp);
    if ((out = fdopen(fd, "w")) == NULL) {
	close(fd);
	rb_fatal("cannot open temporary file: %s", RSTRING(scriptname)->ptr);
    }
    err = eruby_compile(in, out);
    if (in != stdin) fclose(in);
    fclose(out);
    switch (err) {
    case ERUBY_MISSING_END_DELIMITER:
	rb_raise(rb_eSyntaxError, "missing end delimiter");
	break;
    case ERUBY_INVALID_OPTION:
	rb_raise(rb_eSyntaxError, "invalid #! line");
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
    int status;

    scriptname = rb_protect(eruby_compile_file, (VALUE) filename, &status);
    if (state) *state = status;
    if (status)	return Qnil;
    rb_load_protect(scriptname, wrap, &status);
    if (state) *state = status;
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

    eruby_charset = eruby_default_charset = rb_str_new2(ERUBY_DEFAULT_CHARSET);
    rb_str_freeze(eruby_charset);
    rb_global_variable(&eruby_charset);
    rb_global_variable(&eruby_default_charset);
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
