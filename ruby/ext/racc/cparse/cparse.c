/*

    cparse.c
  
    Copyright (c) 1999-2002 Minero Aoki <aamine@loveruby.net>
  
    This library is free software.
    You can distribute/modify this program under the same terms of ruby.

    $Id$

*/

#include <stdio.h>
#include "ruby.h"


/* -----------------------------------------------------------------------
                        Important Constants
----------------------------------------------------------------------- */

#define RACC_VERSION "1.4.2"

#define DEFAULT_TOKEN -1
#define ERROR_TOKEN    1
#define FINAL_TOKEN    0

#define vDEFAULT_TOKEN  INT2FIX(DEFAULT_TOKEN)
#define vERROR_TOKEN    INT2FIX(ERROR_TOKEN)
#define vFINAL_TOKEN    INT2FIX(FINAL_TOKEN)


/* -----------------------------------------------------------------------
                          Global Variables
----------------------------------------------------------------------- */

static VALUE RaccBug;
static VALUE CparseParams;

static ID id_yydebug;
static ID id_nexttoken;
static ID id_onerror;
static ID id_noreduce;
static ID id_catch;
static VALUE sym_raccjump;
static ID id_errstatus;

static ID id_d_shift;
static ID id_d_reduce;
static ID id_d_accept;
static ID id_d_read_token;
static ID id_d_next_state;
static ID id_d_e_pop;


/* -----------------------------------------------------------------------
                              Utils
----------------------------------------------------------------------- */

static ID value_to_id _((VALUE v));
static inline long num_to_long _((VALUE n));

#ifdef ID2SYM
# define id_to_value(i) ID2SYM(i)
#else
# define id_to_value(i) ULONG2NUM(i)
#endif

static ID
value_to_id(v)
    VALUE v;
{
#ifndef SYMBOL_P
#  define SYMBOL_P(v) FIXNUM_P(v)
#endif
    if (! SYMBOL_P(v)) {
        rb_raise(rb_eTypeError, "not symbol");
    }
#ifdef SYM2ID
    return SYM2ID(v);
#else
    return (ID)NUM2ULONG(v);
#endif
}

#ifndef LONG2NUM
#  define LONG2NUM(i) INT2NUM(i)
#endif

static inline long
num_to_long(n)
    VALUE n;
{
    return NUM2LONG(n);
}

#define AREF(s, idx) \
    ((0 <= idx && idx < RARRAY(s)->len) ? RARRAY(s)->ptr[idx] : Qnil)


/* -----------------------------------------------------------------------
                        Parser Stack Interfaces
----------------------------------------------------------------------- */

static VALUE get_stack_tail _((VALUE stack, long len));
static void cut_stack_tail _((VALUE stack, long len));

static VALUE
get_stack_tail(stack, len)
    VALUE stack;
    long len;
{
    if (len < 0) return Qnil;  /* system error */
    if (len > RARRAY(stack)->len) len = RARRAY(stack)->len;
    return rb_ary_new4(len, RARRAY(stack)->ptr + RARRAY(stack)->len - len);
}

static void
cut_stack_tail(stack, len)
    VALUE stack;
    long len;
{
    while (len > 0) {
        rb_ary_pop(stack);
        len--;
    }
}

#define STACK_INIT_LEN 64
#define NEW_STACK() rb_ary_new2(STACK_INIT_LEN)
#define PUSH(s, i) rb_ary_store(s, RARRAY(s)->len, i)
#define POP(s) rb_ary_pop(s)
#define LAST_I(s) \
    ((RARRAY(s)->len > 0) ? RARRAY(s)->ptr[RARRAY(s)->len - 1] : Qnil)
#define GET_TAIL(s, len) get_stack_tail(s, len)
#define CUT_TAIL(s, len) cut_stack_tail(s, len)


/* -----------------------------------------------------------------------
                       struct cparse_params
----------------------------------------------------------------------- */

struct cparse_params {
    VALUE value_v;         /* VALUE version of this struct */

    VALUE parser;          /* parser object */

    VALUE lexer;           /* receiver object of scan iterator */
    ID    lexmid;          /* name of scan iterator method */

    /* state transition tables (never change)
       Using data structure is from Dragon Book 4.9 */
    /* action table */
    VALUE action_table;
    VALUE action_check;
    VALUE action_default;
    VALUE action_pointer;
    /* goto table */
    VALUE goto_table;
    VALUE goto_check;
    VALUE goto_default;
    VALUE goto_pointer;

    long  nt_base;         /* NonTerminal BASE index */
    VALUE reduce_table;    /* reduce data table */
    VALUE token_table;     /* token conversion table */

    /* parser stacks and parameters */
    VALUE state;
    long curstate;
    VALUE vstack;
    VALUE tstack;
    VALUE t;
    long shift_n;
    long reduce_n;
    long ruleno;

    long errstatus;         /* nonzero in error recovering mode */
    long nerr;              /* number of error */

    /* runtime user option */
    int use_result_var;     /* bool */
    int iterator_p;         /* bool */

    VALUE retval;           /* return value of parser routine */
    long fin;               /* parse result status */
#define CP_FIN_ACCEPT  1
#define CP_FIN_EOT     2
#define CP_FIN_CANTPOP 3

    int debug;              /* user level debug */
    int sys_debug;          /* system level debug */

    long i;                 /* table index */
};


/* -----------------------------------------------------------------------
                        Parser Main Routines
----------------------------------------------------------------------- */

static VALUE racc_cparse _((VALUE parser, VALUE arg, VALUE sysdebug));
static VALUE racc_yyparse _((VALUE parser, VALUE lexer, VALUE lexmid,
                             VALUE arg, VALUE sysdebug));

static void call_lexer _((struct cparse_params *v));
static VALUE lexer_iter _((VALUE data));
static VALUE lexer_i _((VALUE block_args, VALUE data, VALUE self));

static VALUE check_array _((VALUE a));
static long check_num _((VALUE n));
static VALUE check_hash _((VALUE h));
static void initialize_params _((struct cparse_params *v,
                                 VALUE parser, VALUE arg,
                                 VALUE lexer, VALUE lexmid));

static void parse_main _((struct cparse_params *v,
                         VALUE tok, VALUE val, int resume));
static void extract_user_token _((struct cparse_params *v,
                                  VALUE block_args, VALUE *tok, VALUE *val));
static void shift _((struct cparse_params* v, long act, VALUE tok, VALUE val));
static int reduce _((struct cparse_params* v, long act));
static VALUE catch_iter _((VALUE dummy));
static VALUE reduce0 _((VALUE block_args, VALUE data, VALUE self));

#ifdef DEBUG
# define D(code) if (v->sys_debug) code
#else
# define D(code)
#endif

static VALUE
racc_cparse(parser, arg, sysdebug)
    VALUE parser, arg, sysdebug;
{
    struct cparse_params params;

    params.sys_debug = RTEST(sysdebug);
    D(puts("start C doparse"));
    initialize_params(&params, parser, arg, Qnil, Qnil);
    params.iterator_p = Qfalse;
    D(puts("params initialized"));
    parse_main(&params, Qnil, Qnil, 0);
    return params.retval;
}

static VALUE
racc_yyparse(parser, lexer, lexmid, arg, sysdebug)
    VALUE parser, lexer, lexmid, arg, sysdebug;
{
    struct cparse_params params;

    params.sys_debug = RTEST(sysdebug);
    D(puts("start C yyparse"));
    initialize_params(&params, parser, arg, lexer, lexmid);
    params.iterator_p = Qtrue;
    D(puts("params initialized"));
    parse_main(&params, Qnil, Qnil, 0);
    call_lexer(&params);
    if (! params.fin) {
        rb_raise(rb_eArgError, "%s() is finished before EndOfToken",
                 rb_id2name(params.lexmid));
    }

    return params.retval;
}

static void
call_lexer(v)
    struct cparse_params *v;
{
    rb_iterate(lexer_iter, v->value_v, lexer_i, v->value_v);
}

static VALUE
lexer_iter(data)
    VALUE data;
{
    struct cparse_params *v;

    Data_Get_Struct(data, struct cparse_params, v);
    rb_funcall(v->lexer, v->lexmid, 0);
    return Qnil;
}

static VALUE
lexer_i(block_args, data, self)
    VALUE block_args, data, self;
{
    struct cparse_params *v;
    VALUE tok, val;

    Data_Get_Struct(data, struct cparse_params, v);
    if (v->fin)
        rb_raise(rb_eArgError, "extra token after EndOfToken");
    extract_user_token(v, block_args, &tok, &val);
    parse_main(v, tok, val, 1);
    if (v->fin && v->fin != CP_FIN_ACCEPT)
       rb_iter_break(); 
    return Qnil;
}

static VALUE
check_array(a)
    VALUE a;
{
    Check_Type(a, T_ARRAY);
    return a;
}

static VALUE
check_hash(h)
    VALUE h;
{
    Check_Type(h, T_HASH);
    return h;
}

static long
check_num(n)
    VALUE n;
{
    return NUM2LONG(n);
}

static void
initialize_params(v, parser, arg, lexer, lexmid)
    struct cparse_params *v;
    VALUE parser, arg, lexer, lexmid;
{
    v->value_v = Data_Wrap_Struct(CparseParams, 0, 0, v);

    v->parser = parser;
    v->lexer = lexer;
    if (! NIL_P(lexmid))
        v->lexmid = value_to_id(lexmid);

    v->debug = RTEST(rb_ivar_get(parser, id_yydebug));

    Check_Type(arg, T_ARRAY);
    if (!(13 <= RARRAY(arg)->len && RARRAY(arg)->len <= 14))
        rb_raise(RaccBug, "[Racc Bug] wrong arg.size %ld", RARRAY(arg)->len);
    v->action_table   = check_array(RARRAY(arg)->ptr[ 0]);
    v->action_check   = check_array(RARRAY(arg)->ptr[ 1]);
    v->action_default = check_array(RARRAY(arg)->ptr[ 2]);
    v->action_pointer = check_array(RARRAY(arg)->ptr[ 3]);
    v->goto_table     = check_array(RARRAY(arg)->ptr[ 4]);
    v->goto_check     = check_array(RARRAY(arg)->ptr[ 5]);
    v->goto_default   = check_array(RARRAY(arg)->ptr[ 6]);
    v->goto_pointer   = check_array(RARRAY(arg)->ptr[ 7]);
    v->nt_base        = check_num  (RARRAY(arg)->ptr[ 8]);
    v->reduce_table   = check_array(RARRAY(arg)->ptr[ 9]);
    v->token_table    = check_hash (RARRAY(arg)->ptr[10]);
    v->shift_n        = check_num  (RARRAY(arg)->ptr[11]);
    v->reduce_n       = check_num  (RARRAY(arg)->ptr[12]);
    if (RARRAY(arg)->len > 13) {
        v->use_result_var = RTEST(RARRAY(arg)->ptr[13]);
    }
    else {
        v->use_result_var = Qtrue;
    }

    v->tstack = v->debug ? NEW_STACK() : Qnil;
    v->vstack = NEW_STACK();
    v->state = NEW_STACK();
    v->curstate = 0;
    PUSH(v->state, INT2FIX(0));
    v->t = INT2FIX(FINAL_TOKEN + 1);   /* must not init to FINAL_TOKEN */
    v->nerr = 0;
    v->errstatus = 0;
    rb_ivar_set(parser, id_errstatus, LONG2NUM(v->errstatus));

    v->retval = Qnil;
    v->fin = 0;

    v->iterator_p = Qfalse;
}

static void
extract_user_token(v, block_args, tok, val)
    struct cparse_params *v;
    VALUE block_args;
    VALUE *tok, *val;
{
    if (NIL_P(block_args)) {
        /* EOF */
        *tok = Qfalse;
        *val = rb_str_new("$", 1);
        return;
    }

    if (TYPE(block_args) != T_ARRAY) {
        rb_raise(rb_eTypeError,
                 "%s() %s %s (must be Array[2])",
                 v->iterator_p ? rb_id2name(v->lexmid) : "next_token",
                 v->iterator_p ? "yielded" : "returned",
                 rb_class2name(CLASS_OF(block_args)));
    }
    if (RARRAY(block_args)->len != 2) {
        rb_raise(rb_eArgError,
                 "%s() %s wrong size of array (%ld for 2)",
                 v->iterator_p ? rb_id2name(v->lexmid) : "next_token",
                 v->iterator_p ? "yielded" : "returned",
                 RARRAY(block_args)->len);
    }
    *tok = AREF(block_args, 0);
    *val = AREF(block_args, 1);
}

#define SHIFT(v,act,tok,val) shift(v,act,tok,val)
#define REDUCE(v,act) do {\
    switch (reduce(v,act)) {  \
      case 0: /* normal */    \
        break;                \
      case 1: /* yyerror */   \
        goto user_yyerror;    \
      case 2: /* yyaccept */  \
        D(puts("u accept"));  \
        goto accept;          \
      default:                \
        break;                \
    }                         \
} while (0)

static void
parse_main(v, tok, val, resume)
    struct cparse_params *v;
    VALUE tok, val;
    int resume;
{
    long act;
    long i;
    int read_next = 1;
    VALUE vact;
    VALUE tmp;

    if (resume)
        goto resume;
    
    while (1) {
        D(puts("enter new loop"));

        D(printf("(act) k1=%ld\n", v->curstate));
        tmp = AREF(v->action_pointer, v->curstate);
        if (NIL_P(tmp)) goto notfound;
        D(puts("(act) pointer[k1] true"));
        i = NUM2LONG(tmp);

        D(printf("read_next=%d\n", read_next));
        if (read_next) {
            if (v->t != vFINAL_TOKEN) {
                /* Now read token really */
                if (v->iterator_p) {
                    /* scan routine is an iterator */
                    D(puts("goto resume..."));
                    if (v->fin)
                        rb_raise(rb_eArgError, "token given after final token");
                    v->i = i;  /* save i */
                    return;
                  resume:
                    D(puts("resume"));
                    i = v->i;  /* load i */
                }
                else {
                    /* scan routine is next_token() */
                    D(puts("next_token"));
                    tmp = rb_funcall(v->parser, id_nexttoken, 0);
                    extract_user_token(v, tmp, &tok, &val);
                }
                /* convert token */
                tmp = rb_hash_aref(v->token_table, tok);
                v->t = NIL_P(tmp) ? vERROR_TOKEN : tmp;
                D(printf("(act) t(k2)=%ld\n", NUM2LONG(v->t)));
                if (v->debug) {
                    rb_funcall(v->parser, id_d_read_token,
                               3, v->t, tok, val);
                }
            }
            read_next = 0;
        }

        i += NUM2LONG(v->t);
        D(printf("(act) i=%ld\n", i));
        if (i < 0) goto notfound;

        vact = AREF(v->action_table, i);
        D(printf("(act) table[i]=%ld\n", NUM2LONG(vact)));
        if (NIL_P(vact)) goto notfound;

        tmp = AREF(v->action_check, i);
        D(printf("(act) check[i]=%ld\n", NUM2LONG(tmp)));
        if (NIL_P(tmp)) goto notfound;
        if (NUM2LONG(tmp) != v->curstate) goto notfound;

        D(puts("(act) found"));
      act_fixed:
        act = NUM2LONG(vact);
        D(printf("act=%ld\n", act));
        goto handle_act;
    
      notfound:
        D(puts("(act) not found: use default"));
        vact = AREF(v->action_default, v->curstate);
        goto act_fixed;

      handle_act:
        if (act > 0 && act < v->shift_n) {
            D(puts("shift"));
            if (v->errstatus > 0) {
                v->errstatus--;
                rb_ivar_set(v->parser, id_errstatus, LONG2NUM(v->errstatus));
            }
            SHIFT(v, act, v->t, val);
            read_next = 1;
        }
        else if (act < 0 && act > -(v->reduce_n)) {
            D(puts("reduce"));
            REDUCE(v, act);
        }
        else if (act == -(v->reduce_n)) {
            goto error;
          error_return:
            ;   /* goto label requires stmt */
        }
        else if (act == v->shift_n) {
            D(puts("accept"));
            goto accept;
        }
        else {
            rb_raise(RaccBug, "[Racc Bug] unknown act value %ld", act);
        }

        if (v->debug) {
            rb_funcall(v->parser, id_d_next_state,
                       2, LONG2NUM(v->curstate), v->state);
        }
    }
    /* not reach */


  accept:
    if (v->debug) rb_funcall(v->parser, id_d_accept, 0);
    v->retval = RARRAY(v->vstack)->ptr[0];
    v->fin = CP_FIN_ACCEPT;
    return;


  error:
    D(printf("error detected, status=%ld\n", v->errstatus));
    if (v->errstatus == 0) {
        v->nerr++;
        rb_funcall(v->parser, id_onerror,
                   3, v->t, val, v->vstack);
    }
  user_yyerror:
    if (v->errstatus == 3) {
        if (v->t == vFINAL_TOKEN) {
            v->retval = Qfalse;
            v->fin = CP_FIN_EOT;
            return;
        }
        read_next = 1;
    }
    v->errstatus = 3;
    rb_ivar_set(v->parser, id_errstatus, LONG2NUM(v->errstatus));

    /* check if we can shift/reduce error token */
    D(printf("(err) k1=%ld\n", v->curstate));
    D(printf("(err) k2=%d (error)\n", ERROR_TOKEN));
    while (1) {
        tmp = AREF(v->action_pointer, v->curstate);
        if (NIL_P(tmp)) goto e_notfound;
        D(puts("(err) pointer[k1] true"));

        i = NUM2LONG(tmp) + ERROR_TOKEN;
        D(printf("(err) i=%ld\n", i));
        if (i < 0) goto e_notfound;

        vact = AREF(v->action_table, i);
        if (NIL_P(vact)) {
            D(puts("(err) table[i] == nil"));
            goto e_notfound;
        }
        D(printf("(err) table[i]=%ld\n", NUM2LONG(vact)));

        tmp = AREF(v->action_check, i);
        if (NIL_P(tmp)) {
            D(puts("(err) check[i] == nil"));
            goto e_notfound;
        }
        if (NUM2LONG(tmp) != v->curstate) {
            D(puts("(err) check[i]!=k1 or nil"));
            goto e_notfound;
        }

        D(puts("(err) found: can handle error token"));
        act = NUM2LONG(vact);
        break;
          
      e_notfound:
        D(puts("(err) not found: can't handle error token; pop"));

        if (RARRAY(v->state)->len == 0) {
            v->retval = Qnil;
            v->fin = CP_FIN_CANTPOP;
            return;
        }
        POP(v->state);
        POP(v->vstack);
        v->curstate = num_to_long(LAST_I(v->state));
        if (v->debug) {
            POP(v->tstack);
            rb_funcall(v->parser, id_d_e_pop,
                       3, v->state, v->tstack, v->vstack);
        }
    }

    /* shift/reduce error token */
    if (act > 0 && act < v->shift_n) {
        D(puts("e shift"));
        SHIFT(v, act, ERROR_TOKEN, val);
    }
    else if (act < 0 && act > -(v->reduce_n)) {
        D(puts("e reduce"));
        REDUCE(v, act);
    }
    else if (act == v->shift_n) {
        D(puts("e accept"));
        goto accept;
    }
    else {
        rb_raise(RaccBug, "[Racc Bug] unknown act value %ld", act);
    }
    goto error_return;
}

static void
shift(v, act, tok, val)
    struct cparse_params *v;
    long act;
    VALUE tok, val;
{
    PUSH(v->vstack, val);
    if (v->debug) {
        PUSH(v->tstack, tok);
        rb_funcall(v->parser, id_d_shift,
                   3, tok, v->tstack, v->vstack);
    }
    v->curstate = act;
    PUSH(v->state, LONG2NUM(v->curstate));
}

static int
reduce(v, act)
    struct cparse_params *v;
    long act;
{
    VALUE code;
    v->ruleno = -act * 3;
    code = rb_iterate(catch_iter, Qnil, reduce0, v->value_v);
    v->errstatus = num_to_long(rb_ivar_get(v->parser, id_errstatus));
    return NUM2INT(code);
}

static VALUE
catch_iter(dummy)
    VALUE dummy;
{
    return rb_funcall(rb_mKernel, id_catch, 1, sym_raccjump);
}

static VALUE
reduce0(val, data, self)
    VALUE val, data, self;
{
    struct cparse_params *v;
    VALUE reduce_to, reduce_len, method_id;
    long len;
    ID mid;
    VALUE tmp, tmp_t, tmp_v;
    long i, k1, k2;
    VALUE goto_state;

    Data_Get_Struct(data, struct cparse_params, v);
    reduce_len = RARRAY(v->reduce_table)->ptr[v->ruleno];
    reduce_to  = RARRAY(v->reduce_table)->ptr[v->ruleno+1];
    method_id  = RARRAY(v->reduce_table)->ptr[v->ruleno+2];
    len = NUM2LONG(reduce_len);
    mid = value_to_id(method_id);

    /* call action */
    if (len == 0) {
        tmp = Qnil;
        if (mid != id_noreduce)
            tmp_v = rb_ary_new();
        if (v->debug)
            tmp_t = rb_ary_new();
    }
    else {
        if (mid != id_noreduce) {
            tmp_v = GET_TAIL(v->vstack, len);
            tmp = RARRAY(tmp_v)->ptr[0];
        }
        else {
            tmp = RARRAY(v->vstack)->ptr[ RARRAY(v->vstack)->len - len ];
        }
        CUT_TAIL(v->vstack, len);
        if (v->debug) {
            tmp_t = GET_TAIL(v->tstack, len);
            CUT_TAIL(v->tstack, len);
        }
        CUT_TAIL(v->state, len);
    }
    if (mid != id_noreduce) {
        if (v->use_result_var) {
            tmp = rb_funcall(v->parser, mid,
                             3, tmp_v, v->vstack, tmp);
        }
        else {
            tmp = rb_funcall(v->parser, mid,
                             2, tmp_v, v->vstack);
        }
    }

    /* then push result */
    PUSH(v->vstack, tmp);
    if (v->debug) {
        PUSH(v->tstack, reduce_to);
        rb_funcall(v->parser, id_d_reduce,
                   4, tmp_t, reduce_to, v->tstack, v->vstack);
    }

    /* calculate transition state */
    if (RARRAY(v->state)->len == 0)
        rb_raise(RaccBug, "state stack unexpected empty");
    k2 = num_to_long(LAST_I(v->state));
    k1 = num_to_long(reduce_to) - v->nt_base;
    D(printf("(goto) k1=%ld\n", k1));
    D(printf("(goto) k2=%ld\n", k2));

    tmp = AREF(v->goto_pointer, k1);
    if (NIL_P(tmp)) goto notfound;

    i = NUM2LONG(tmp) + k2;
    D(printf("(goto) i=%ld\n", i));
    if (i < 0) goto notfound;

    goto_state = AREF(v->goto_table, i);
    if (NIL_P(goto_state)) {
        D(puts("(goto) table[i] == nil"));
        goto notfound;
    }
    D(printf("(goto) table[i]=%ld (goto_state)\n", NUM2LONG(goto_state)));

    tmp = AREF(v->goto_check, i);
    if (NIL_P(tmp)) {
        D(puts("(goto) check[i] == nil"));
        goto notfound;
    }
    if (tmp != LONG2NUM(k1)) {
        D(puts("(goto) check[i] != table[i]"));
        goto notfound;
    }
    D(printf("(goto) check[i]=%ld\n", NUM2LONG(tmp)));

    D(puts("(goto) found"));
  transit:
    PUSH(v->state, goto_state);
    v->curstate = NUM2LONG(goto_state);
    return INT2FIX(0);

  notfound:
    D(puts("(goto) not found: use default"));
    /* overwrite `goto-state' by default value */
    goto_state = AREF(v->goto_default, k1);
    goto transit;
}


/* -----------------------------------------------------------------------
                          Ruby Interface
----------------------------------------------------------------------- */

void
Init_cparse()
{
    VALUE Racc;
    VALUE Parser;
    ID id_racc = rb_intern("Racc");

    if (rb_const_defined(rb_cObject, id_racc)) {
        Racc = rb_const_get(rb_cObject, id_racc);
        Parser = rb_const_get_at(Racc, rb_intern("Parser"));
    }
    else {
        Racc = rb_define_module("Racc");
        Parser = rb_define_class_under(Racc, "Parser", rb_cObject);
    }
    rb_define_private_method(Parser, "_racc_do_parse_c", racc_cparse, 2);
    rb_define_private_method(Parser, "_racc_yyparse_c", racc_yyparse, 4);
    rb_define_const(Parser, "Racc_Runtime_Core_Version_C",
                    rb_str_new2(RACC_VERSION));
    rb_define_const(Parser, "Racc_Runtime_Core_Id_C",
        rb_str_new2("$Id$"));

    CparseParams = rb_define_class_under(Racc, "CparseParams", rb_cObject);

    RaccBug = rb_eRuntimeError;

    id_yydebug      = rb_intern("@yydebug");
    id_nexttoken    = rb_intern("next_token");
    id_onerror      = rb_intern("on_error");
    id_noreduce     = rb_intern("_reduce_none");
    id_catch        = rb_intern("catch");
    id_errstatus    = rb_intern("@racc_error_status");
    sym_raccjump    = id_to_value(rb_intern("racc_jump"));

    id_d_shift       = rb_intern("racc_shift");
    id_d_reduce      = rb_intern("racc_reduce");
    id_d_accept      = rb_intern("racc_accept");
    id_d_read_token  = rb_intern("racc_read_token");
    id_d_next_state  = rb_intern("racc_next_state");
    id_d_e_pop       = rb_intern("racc_e_pop");
}
