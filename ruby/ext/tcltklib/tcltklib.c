/*
 *	tcltklib.c
 *		Aug. 27, 1997	Y. Shigehiro
 *		Oct. 24, 1997	Y. Matsumoto
 */

#include "ruby.h"
#include "sig.h"
#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include <tk.h>

/* for debug */

#define DUMP1(ARG1) if (debug) { fprintf(stderr, "tcltklib: %s\n", ARG1);}
#define DUMP2(ARG1, ARG2) if (debug) { fprintf(stderr, "tcltklib: ");\
fprintf(stderr, ARG1, ARG2); fprintf(stderr, "\n"); }
/*
#define DUMP1(ARG1)
#define DUMP2(ARG1, ARG2)
*/

/* from tkAppInit.c */

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

/*---- module TclTkLib ----*/

static VALUE thread_safe = Qnil;

/* execute Tk_MainLoop */
static VALUE
lib_mainloop(VALUE self)
{
    int old_trapflg;
    int flags = RTEST(thread_safe)?TCL_DONT_WAIT:0;

    DUMP1("start Tk_Mainloop");
    while (Tk_GetNumMainWindows() > 0) {
	old_trapflg = trap_immediate;
	trap_immediate = 1;
        Tcl_DoOneEvent(flags);
	trap_immediate = old_trapflg;
	CHECK_INTS;
	flags = (thread_safe == 0 || thread_safe == Qnil)?0:TCL_DONT_WAIT;
    }
    DUMP1("stop Tk_Mainloop");

    return Qnil;
}

/*---- class TclTkIp ----*/
struct tcltkip {
    Tcl_Interp *ip;		/* the interpreter */
    int return_value;		/* return value */
};

/* Tcl command `ruby' */
static VALUE
ip_eval_rescue(VALUE *failed, VALUE einfo)
{
    *failed = einfo;
    return Qnil;
}

static int
ip_ruby(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
    VALUE res;
    int old_trapflg;
    VALUE failed = 0;

    /* ruby command has 1 arg. */
    if (argc != 2) {
	ArgError("wrong # of arguments (%d for 1)", argc);
    }

    /* evaluate the argument string by ruby */
    DUMP2("rb_eval_string(%s)", argv[1]);
    old_trapflg = trap_immediate;
    trap_immediate = 0;
    res = rb_rescue(rb_eval_string, argv[1], ip_eval_rescue, &failed);
    trap_immediate = old_trapflg;

    if (failed) {
	Tcl_AppendResult(interp, RSTRING(failed)->ptr, (char*)NULL);
	return TCL_ERROR;
    }

    /* result must be string or nil */
    if (NIL_P(res)) {
	DUMP1("(rb_eval_string result) nil");
	return TCL_OK;
    }
    Check_Type(res, T_STRING);

    /* copy result to the tcl interpreter */
    DUMP2("(rb_eval_string result) %s", RSTRING(res)->ptr);
    DUMP1("Tcl_AppendResult");
    Tcl_AppendResult(interp, RSTRING(res)->ptr, (char *)NULL);

    return TCL_OK;
}

/* destroy interpreter */
static void
ip_free(struct tcltkip *ptr)
{
    DUMP1("Tcl_DeleteInterp");
    Tcl_DeleteInterp(ptr->ip);
}

/* create and initialize interpreter */
static VALUE
ip_new(VALUE self)
{
    struct tcltkip *ptr;	/* tcltkip data struct */
    VALUE obj;			/* newly created object */

    /* create object */
    obj = Data_Make_Struct(self, struct tcltkip, 0, ip_free, ptr);
    ptr->return_value = 0;

    /* from Tk_Main() */
    DUMP1("Tcl_CreateInterp");
    ptr->ip = Tcl_CreateInterp();

    /* from Tcl_AppInit() */
    DUMP1("Tcl_Init");
    if (Tcl_Init(ptr->ip) == TCL_ERROR) {
	Fail("Tcl_Init");
    }
    DUMP1("Tk_Init");
    if (Tk_Init(ptr->ip) == TCL_ERROR) {
	Fail("Tk_Init");
    }
    DUMP1("Tcl_StaticPackage(\"Tk\")");
    Tcl_StaticPackage(ptr->ip, "Tk", Tk_Init,
		      (Tcl_PackageInitProc *) NULL);

    /* add ruby command to the interpreter */
    DUMP1("Tcl_CreateCommand(\"ruby\")");
    Tcl_CreateCommand(ptr->ip, "ruby", ip_ruby, (ClientData *)NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    return obj;
}

/* eval string in tcl by Tcl_Eval() */
static VALUE
ip_eval(VALUE self, VALUE str)
{
    char *buf;			/* Tcl_Eval requires re-writable string region */
    struct tcltkip *ptr;	/* tcltkip data struct */

    /* get the data struct */
    Data_Get_Struct(self, struct tcltkip, ptr);

    /* call Tcl_Eval() */
    Check_Type(str, T_STRING);
    buf = ALLOCA_N(char,RSTRING(str)->len+1);
    strcpy(buf, RSTRING(str)->ptr);
    DUMP2("Tcl_Eval(%s)", buf);
    ptr->return_value = Tcl_Eval(ptr->ip, buf);
    if (ptr->return_value == TCL_ERROR) {
	Fail(ptr->ip->result);
    }
    DUMP2("(TCL_Eval result) %d", ptr->return_value);

    /* pass back the result (as string) */
    return(str_new2(ptr->ip->result));
}

/* get return code from Tcl_Eval() */
static VALUE
ip_retval(VALUE self)
{
    struct tcltkip *ptr;	/* tcltkip data struct */

    /* get the data strcut */
    Data_Get_Struct(self, struct tcltkip, ptr);

    return (INT2FIX(ptr->return_value));
}

/*---- initialization ----*/
void Init_tcltklib()
{
    extern VALUE rb_argv0;	/* the argv[0] */

    VALUE lib = rb_define_module("TclTkLib");
    VALUE ip = rb_define_class("TclTkIp", cObject);

    rb_define_module_function(lib, "mainloop", lib_mainloop, 0);

    rb_define_singleton_method(ip, "new", ip_new, 0);
    rb_define_method(ip, "_eval", ip_eval, 1);
    rb_define_method(ip, "_return_value", ip_retval, 0);
    rb_define_method(ip, "mainloop", lib_mainloop, 0);

    /*---- initialize tcl/tk libraries ----*/
    /* from Tk_Main() */
    DUMP1("Tcl_FindExecutable");
    Tcl_FindExecutable(RSTRING(rb_argv0)->ptr);

    rb_define_variable("$tk_thread_safe", &thread_safe);
}

/* eof */
