/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 */

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

#include "ruby.h"
#include "eruby.h"
#include "config.h"

EXTERN VALUE rb_stdin;

static VALUE mERuby;
static VALUE cERubyCompiler;
static VALUE eERubyCompileError;

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

const char *eruby_version()
{
    return ERUBY_VERSION;
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

typedef struct eruby_compiler {
    VALUE output;
    int   sourceline;
    VALUE (*lex_gets)(struct eruby_compiler *compiler);
    VALUE lex_input;
    VALUE lex_lastline;
    char *lex_pbeg;
    char *lex_p;
    char *lex_pend;
    int   lex_gets_ptr;
    char  buf[BUFSIZ];
    long  buf_len;
} eruby_compiler_t;

static void eruby_compiler_mark(eruby_compiler_t *compiler)
{
    rb_gc_mark(compiler->lex_input);
    rb_gc_mark(compiler->lex_lastline);
    rb_gc_mark(compiler->output);
}

static VALUE eruby_compiler_s_new(VALUE self)
{
    VALUE obj;
    eruby_compiler_t *compiler;

    obj = Data_Make_Struct(self, eruby_compiler_t,
			   eruby_compiler_mark, free, compiler);
    compiler->output = Qnil;
    compiler->sourceline = 0;
    compiler->lex_gets = NULL;
    compiler->lex_input = Qnil;
    compiler->lex_lastline = Qnil;
    compiler->lex_pbeg = NULL;
    compiler->lex_p = NULL;
    compiler->lex_pend = NULL;
    compiler->lex_gets_ptr = 0;
    compiler->buf_len = 0;
    return obj;
}

VALUE eruby_compiler_new()
{
    return eruby_compiler_s_new(cERubyCompiler);
}

static VALUE lex_str_gets(eruby_compiler_t *compiler)
{
    VALUE s = compiler->lex_input;
    char *beg, *end, *pend;

    beg = RSTRING(s)->ptr;
    if (compiler->lex_gets_ptr) {
	if (RSTRING(s)->len == compiler->lex_gets_ptr)
	    return Qnil;
	beg += compiler->lex_gets_ptr;
    }
    pend = RSTRING(s)->ptr + RSTRING(s)->len;
    end = beg;
    while (end < pend) {
	if (*end++ == '\n') break;
    }
    compiler->lex_gets_ptr = end - RSTRING(s)->ptr;
    return rb_str_new(beg, end - beg);
}

static VALUE lex_io_gets(eruby_compiler_t *compiler)
{
    return rb_io_gets(compiler->lex_input);
}

static inline int nextc(eruby_compiler_t *compiler)
{
    int c;

    if (compiler->lex_p == compiler->lex_pend) {
	if (compiler->lex_input) {
	    VALUE v = (*compiler->lex_gets)(compiler);

	    if (NIL_P(v)) return EOF;
	    compiler->sourceline++;
	    compiler->lex_pbeg = compiler->lex_p = RSTRING(v)->ptr;
	    compiler->lex_pend = compiler->lex_p + RSTRING(v)->len;
	    compiler->lex_lastline = v;
	}
	else {
	    compiler->lex_lastline = Qnil;
	    return EOF;
	}
    }
    c = (unsigned char)*compiler->lex_p++;
    if (c == '\r' && compiler->lex_p <= compiler->lex_pend && *compiler->lex_p == '\n') {
	compiler->lex_p++;
	c = '\n';
    }

    return c;
}

static void pushback(eruby_compiler_t *compiler, int c)
{
    if (c == EOF) return;
    compiler->lex_p--;
}

static void flushbuf(eruby_compiler_t *compiler)
{
    rb_str_cat(compiler->output, compiler->buf, compiler->buf_len);
    compiler->buf_len = 0;
}

static void output(eruby_compiler_t *compiler, const char *s, long len)
{
    if (len > BUFSIZ) {
	rb_str_cat(compiler->output, s, len);
	return;
    }
    if (compiler->buf_len + len > BUFSIZ)
	flushbuf(compiler);
    memcpy(compiler->buf + compiler->buf_len, s, len);
    compiler->buf_len += len;
}

static inline void output_char(eruby_compiler_t *compiler, int c)
{
    char ch = c & 0xff;

    if (compiler->buf_len == BUFSIZ)
	flushbuf(compiler);
    compiler->buf[compiler->buf_len++] = ch;
}

#define output_literal(compiler, s) output(compiler, s, sizeof(s) - 1)

static void parse_embedded_program(eruby_compiler_t *compiler,
				   enum embedded_program_type type)
{
    int c, prevc = EOF;

    if (type == EMBEDDED_EXPR)
	output_literal(compiler, "print((");
    for (;;) {
	c = nextc(compiler);
      again:
	if (c == ERUBY_END_DELIMITER[0]) {
	    c = nextc(compiler);
	    if (c == ERUBY_END_DELIMITER[1]) {
		if (prevc == ERUBY_END_DELIMITER[0]) {
		    if (type != EMBEDDED_COMMENT)
			output(compiler, ERUBY_END_DELIMITER + 1, 1);
		    prevc = ERUBY_END_DELIMITER[1];
		    continue;
		}
		if (type == EMBEDDED_EXPR)
		    output_literal(compiler, ")); ");
		else if (type == EMBEDDED_STMT && prevc != '\n')
		    output_literal(compiler, "; ");
		return;
	    }
	    else if (c == EOF) {
		rb_raise(eERubyCompileError, "%d:missing end delimiter",
			 compiler->sourceline);
	    }
	    else {
		if (type != EMBEDDED_COMMENT)
		    output(compiler, ERUBY_END_DELIMITER, 1);
		prevc = ERUBY_END_DELIMITER[0];
		goto again;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		rb_raise(eERubyCompileError, "%d:missing end delimiter",
			 compiler->sourceline);
		break;
	    case '\n':
		output_char(compiler, c);
		prevc = c;
		break;
	    default:
		if (type != EMBEDDED_COMMENT)
		    output_char(compiler, c);
		prevc = c;
		break;
	    }
	}
    }
    rb_raise(eERubyCompileError, "%d:missing end delimiter",
	     compiler->sourceline);
}

static void parse_embedded_line(eruby_compiler_t *compiler)
{
    int c;

    for (;;) {
	c = nextc(compiler);
	switch (c) {
	case EOF:
	    rb_raise(eERubyCompileError, "%d:missing end delimiter",
		     compiler->sourceline);
	case '\n':
	    output_char(compiler, c);
	    return;
	default:
	    output_char(compiler, c);
	    break;
	}
    }
    rb_raise(eERubyCompileError, "%d:missing end delimiter",
	     compiler->sourceline);
}

static VALUE eruby_compile(eruby_compiler_t *compiler)
{
    int c, prevc = EOF;

    c = nextc(compiler);
    if (c == '#') {
	c = nextc(compiler);
	if (c == '!') {
	    unsigned char *p;
	    char *argv[2];
	    char *line = RSTRING(compiler->lex_lastline)->ptr;

	    if (line[strlen(line) - 1] == '\n') {
		line[strlen(line) - 1] = '\0';
		output_literal(compiler, "\n");
	    }
	    argv[0] = "eruby";
	    p = line;
	    while (isspace(*p)) p++;
	    while (*p && !isspace(*p)) p++;
	    while (isspace(*p)) p++;
	    argv[1] = p;
	    if (eruby_parse_options(2, argv) != 0) {
		rb_raise(eERubyCompileError, "invalid #! line");
	    }
	    compiler->lex_p = compiler->lex_pend;
	}
	else {
	    pushback(compiler, c);
	    pushback(compiler, '#');
	}
    }
    else {
	pushback(compiler, c);
    }

    for (;;) {
	c = nextc(compiler);
      again:
	if (c == ERUBY_BEGIN_DELIMITER[0]) {
	    c = nextc(compiler);
	    if (c == ERUBY_BEGIN_DELIMITER[1]) {
		c = nextc(compiler);
		if (c == EOF) {
		    rb_raise(eERubyCompileError, "%d:missing end delimiter",
			     compiler->sourceline);
		}
		else if (c == ERUBY_BEGIN_DELIMITER[1]) { /* <%% => <% */
		    if (prevc < 0) output_literal(compiler, "print \"");
		    output(compiler, ERUBY_BEGIN_DELIMITER, 2);
		    prevc = ERUBY_BEGIN_DELIMITER[1];
		    continue;
		}
		else {
		    if (prevc >= 0)
			output_literal(compiler, "\"; ");
		    if (c == ERUBY_COMMENT_CHAR) {
			parse_embedded_program(compiler, EMBEDDED_COMMENT);
		    }
		    else if (c == ERUBY_EXPR_CHAR) {
			parse_embedded_program(compiler, EMBEDDED_EXPR);
		    }
		    else {
			pushback(compiler, c);
			parse_embedded_program(compiler, EMBEDDED_STMT);
		    }
		    prevc = EOP;
		}
	    } else {
		if (prevc < 0) output_literal(compiler, "print \"");
		output(compiler, ERUBY_BEGIN_DELIMITER, 1);
		prevc = ERUBY_BEGIN_DELIMITER[0];
		goto again;
	    }
	}
	else if (c == ERUBY_LINE_BEG_CHAR && prevc == EOF) {
	    c = nextc(compiler);
	    if (c == EOF) {
		rb_raise(eERubyCompileError, "%d:missing end delimiter",
			 compiler->sourceline);
	    }
	    else if (c == ERUBY_LINE_BEG_CHAR) { /* %% => % */
		if (prevc < 0) output_literal(compiler, "print \"");
		output_char(compiler, c);
		prevc = c;
	    }
	    else {
		pushback(compiler, c);
		parse_embedded_line(compiler);
		prevc = EOF;
	    }
	}
	else {
	    switch (c) {
	    case EOF:
		goto end;
	    case '\n':
		if (prevc < 0) output_literal(compiler, "print \"");
		output_literal(compiler, "\\n\"\n");
		prevc = EOF;
		break;
	    case '\t':
		if (prevc < 0) output_literal(compiler, "print \"");
		output_literal(compiler, "\\t");
		prevc = c;
		break;
	    case '\\':
	    case '"':
	    case '#':
		if (prevc < 0) output_literal(compiler, "print \"");
		output_literal(compiler, "\\");
		output_char(compiler, c);
		prevc = c;
		break;
	    default:
		if (prevc < 0) output_literal(compiler, "print \"");
		output_char(compiler, c);
		prevc = c;
		break;
	    }
	}
    }
  end:
    if (prevc != EOF) {
	output_literal(compiler, "\"");
    }
    if (compiler->buf_len > 0)
	flushbuf(compiler);
    return compiler->output;
}

VALUE eruby_compiler_compile_string(VALUE self, VALUE s)
{
    eruby_compiler_t *compiler;

    Check_Type(s, T_STRING);
    Data_Get_Struct(self, eruby_compiler_t, compiler);
    compiler->output = rb_str_new("", 0);
    compiler->lex_gets = lex_str_gets;
    compiler->lex_gets_ptr = 0;
    compiler->lex_input = s;
    compiler->lex_pbeg = compiler->lex_p = compiler->lex_pend = 0;
    compiler->buf_len = 0;
    compiler->sourceline = 0;
    return eruby_compile(compiler);
}

VALUE eruby_compiler_compile_file(VALUE self, VALUE file)
{
    eruby_compiler_t *compiler;

    Check_Type(file, T_FILE);
    Data_Get_Struct(self, eruby_compiler_t, compiler);
    compiler->output = rb_str_new("", 0);
    compiler->lex_gets = lex_io_gets;
    compiler->lex_input = file;
    compiler->lex_pbeg = compiler->lex_p = compiler->lex_pend = 0;
    compiler->buf_len = 0;
    compiler->sourceline = 0;
    return eruby_compile(compiler);
}

typedef struct compile_arg {
    VALUE compiler;
    VALUE input;
} compile_arg_t;

static VALUE eruby_compile_file(compile_arg_t *arg)
{
    return eruby_compiler_compile_file(arg->compiler, arg->input);
}

VALUE eruby_load(char *filename, int wrap, int *state)
{
    VALUE compiler;
    VALUE code;
    VALUE f;
    compile_arg_t arg;
    int status;

    if (strcmp(filename, "-") == 0) {
	f = rb_stdin;
    }
    else {
	f = rb_file_open(filename, "r");
    }
    eruby_noheader = 0;
    eruby_charset = eruby_default_charset;
    compiler = eruby_compiler_new();
    arg.compiler = compiler;
    arg.input = f;
    code = rb_protect(eruby_compile_file, (VALUE) &arg, &status);
    if (state) *state = status;
    if (status)	return Qnil;
    if (wrap) {
	rb_eval_string_wrap(STR2CSTR(code), &status);
    }
    else {
	rb_eval_string_protect(STR2CSTR(code), &status);
    }
    if (state) *state = status;
    if (f != rb_stdin)
	rb_io_close(f);
    return code;
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

static VALUE eruby_get_default_charset(VALUE self)
{
    return eruby_default_charset;
}

static VALUE eruby_set_default_charset(VALUE self, VALUE val)
{
    Check_Type(val, T_STRING);
    eruby_default_charset = val;
    return val;
}

static VALUE eruby_import(VALUE self, VALUE filename)
{
    VALUE compiler, file, code;

    compiler = eruby_compiler_new();
    file = rb_file_open(STR2CSTR(filename), "r");
    code = eruby_compiler_compile_file(compiler, file);
    rb_eval_string(STR2CSTR(code));
    return Qnil;
}

void eruby_init()
{
    rb_define_virtual_variable("$NOHEADER", noheader_getter, noheader_setter);

    mERuby = rb_define_module("ERuby");
    rb_define_singleton_method(mERuby, "noheader", eruby_get_noheader, 0);
    rb_define_singleton_method(mERuby, "noheader=", eruby_set_noheader, 1);
    rb_define_singleton_method(mERuby, "charset", eruby_get_charset, 0);
    rb_define_singleton_method(mERuby, "charset=", eruby_set_charset, 1);
    rb_define_singleton_method(mERuby, "default_charset",
			       eruby_get_default_charset, 0);
    rb_define_singleton_method(mERuby, "default_charset=",
			       eruby_set_default_charset, 1);
    rb_define_singleton_method(mERuby, "import", eruby_import, 1);

    cERubyCompiler = rb_define_class_under(mERuby, "Compiler", rb_cObject);
    rb_define_singleton_method(cERubyCompiler, "new", eruby_compiler_s_new, 0);
    rb_define_method(cERubyCompiler, "compile_string",
		     eruby_compiler_compile_string, 1);
    rb_define_method(cERubyCompiler, "compile_file",
		     eruby_compiler_compile_file, 1);

    eERubyCompileError = rb_define_class_under(mERuby, "CompileError",
					       rb_eStandardError);

    eruby_charset = eruby_default_charset = rb_str_new2(ERUBY_DEFAULT_CHARSET);
    rb_str_freeze(eruby_charset);
    rb_global_variable(&eruby_charset);
    rb_global_variable(&eruby_default_charset);

    rb_provide("eruby");
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
