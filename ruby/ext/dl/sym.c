/* -*- C -*-
 * $Id$
 */

#include <ruby.h>
#include "dl.h"

VALUE rb_cDLSymbol;

static const char *
char2type(int ch)
{
  switch (ch) {
  case '0':
    return "void";
  case 'P':
    return "void *";
  case 'p':
    return "void *";
  case 'C':
    return "char";
  case 'c':
    return "char *";
  case 'H':
    return "short";
  case 'h':
    return "short *";
  case 'I':
    return "int";
  case 'i':
    return "int *";
  case 'L':
    return "long";
  case 'l':
    return "long *";
  case 'F':
    return "double";
  case 'f':
    return "double *";
  case 'D':
    return "double";
  case 'd':
    return "double *";
  case 'S':
    return "const char *";
  case 's':
    return "char *";
  case 'A':
    return "[]";
  case 'a':
    return "[]"; /* ?? */
  };
  return NULL;
}

void
dlsym_free(struct sym_data *data)
{
  if( data->name ){
    DEBUG_CODE({
      printf("dlsym_free(): free(data->name:%s)\n",data->name);
    });
    free(data->name);
  };
  if( data->type ){
    DEBUG_CODE({
      printf("dlsym_free(): free(data->type:%s)\n",data->type);
    });
    free(data->type);
  };
}

VALUE
rb_dlsym_new(void (*func)(), const char *name, const char *type)
{
  VALUE val;
  struct sym_data *data;
  const char *ptype;

  if( !type || !type[0] ){
    return rb_dlptr_new((void*)func, 0, 0);
  };

  for( ptype = type; *ptype; ptype ++ ){
    if( ! char2type(*ptype) ){
      rb_raise(rb_eDLTypeError, "unknown type specifier '%c'", *ptype);
    };
  };

  if( func ){
    val = Data_Make_Struct(rb_cDLSymbol, struct sym_data, 0, dlsym_free, data);
    data->func = func;
    data->name = name ? strdup(name) : NULL;
    data->type = type ? strdup(type) : NULL;
    data->len  = type ? strlen(type) : 0;
#if !(defined(DLSTACK))
    if( data->len - 1 > MAX_ARG ){
      rb_raise(rb_eDLError, "maximum number of arguments is %d.", MAX_ARG);
    };
#endif
  }
  else{
    val = Qnil;
  };

  return val;
}

freefunc_t
rb_dlsym2csym(VALUE val)
{
  struct sym_data *data;
  freefunc_t func;

  if( rb_obj_is_kind_of(val, rb_cDLSymbol) ){
    Data_Get_Struct(val, struct sym_data, data);
    func = data->func;
  }
  else if( val == Qnil ){
    func = NULL;
  }
  else{
    rb_raise(rb_eTypeError, "DL::Symbol was expected");
  };

  return func;
}

VALUE
rb_dlsym_s_allocate(VALUE klass)
{
  VALUE obj;
  struct sym_data *data;

  obj = Data_Make_Struct(klass, struct sym_data, 0, dlsym_free, data);
  data->func = 0;
  data->name = 0;
  data->type = 0;
  data->len  = 0;

  return obj;
}

VALUE
rb_dlsym_initialize(int argc, VALUE argv[], VALUE self)
{
  VALUE addr, name, type;
  VALUE val;
  struct sym_data *data;
  void *saddr;
  const char *sname, *stype;

  rb_scan_args(argc, argv, "12", &addr, &name, &type);

  saddr = (void*)(DLNUM2LONG(rb_Integer(addr)));
  sname = NIL_P(name) ? NULL : StringValuePtr(name);
  stype = NIL_P(type) ? NULL : StringValuePtr(type);

  if( saddr ){
    Data_Get_Struct(self, struct sym_data, data);
    if( data->name ) free(data->name);
    if( data->type ) free(data->type);
    data->func = saddr;
    data->name = sname ? strdup(sname) : 0;
    data->type = stype ? strdup(stype) : 0;
    data->len  = stype ? strlen(stype) : 0;
  }

  return Qnil;
}

VALUE
rb_s_dlsym_char2type(VALUE self, VALUE ch)
{
  const char *type;

  type = char2type(StringValuePtr(ch)[0]);

  if (type == NULL)
    return Qnil;
  else
    return rb_str_new2(type);
}

VALUE
rb_dlsym_name(VALUE self)
{
  struct sym_data *sym;

  Data_Get_Struct(self, struct sym_data, sym);
  return sym->name ? rb_tainted_str_new2(sym->name) : Qnil;
}

VALUE
rb_dlsym_proto(VALUE self)
{
  struct sym_data *sym;

  Data_Get_Struct(self, struct sym_data, sym);
  return sym->type ? rb_tainted_str_new2(sym->type) : Qnil;
}

VALUE
rb_dlsym_cproto(VALUE self)
{
  struct sym_data *sym;
  const char *ptype, *typestr;
  size_t len;
  VALUE val;

  Data_Get_Struct(self, struct sym_data, sym);

  ptype = sym->type;

  if( ptype ){
    typestr = char2type(*ptype++);
    len = strlen(typestr);
    
    val = rb_tainted_str_new(typestr, len);
    if (typestr[len - 1] != '*')
      rb_str_cat(val, " ", 1);

    if( sym->name ){
      rb_str_cat2(val, sym->name);
    }
    else{
      rb_str_cat2(val, "(null)");
    };
    rb_str_cat(val, "(", 1);
    
    while (*ptype) {
      const char *ty = char2type(*ptype++);
      rb_str_cat2(val, ty);
      if (*ptype)
	rb_str_cat(val, ", ", 2);
    }

    rb_str_cat(val, ");", 2);
  }
  else{
    val = rb_tainted_str_new2("void (");
    if( sym->name ){
      rb_str_cat2(val, sym->name);
    }
    else{
      rb_str_cat2(val, "(null)");
    };
    rb_str_cat2(val, ")()");
  };

  return val;
}

VALUE
rb_dlsym_inspect(VALUE self)
{
  VALUE proto;
  VALUE val;
  char  *str;
  int str_size;
  struct sym_data *sym;

  Data_Get_Struct(self, struct sym_data, sym);
  proto = rb_dlsym_cproto(self);

  str_size = RSTRING(proto)->len + 100;
  str = dlmalloc(str_size);
  snprintf(str, str_size - 1,
	   "#<DL::Symbol:0x%x func=0x%x '%s'>",
	   sym, sym->func, RSTRING(proto)->ptr);
  val = rb_tainted_str_new2(str);
  dlfree(str);

  return val;
}

static int
stack_size(struct sym_data *sym)
{
  int i;
  int size;

  size = 0;
  for( i=1; i < sym->len; i++ ){
    switch(sym->type[i]){
    case 'C':
    case 'H':
    case 'I':
    case 'L':
      size += sizeof(long);
      break;
    case 'F':
      size += sizeof(float);
      break;
    case 'D':
      size += sizeof(double);
      break;
    case 'c':
    case 'h':
    case 'i':
    case 'l':
    case 'f':
    case 'd':
    case 'p':
    case 'P':
    case 's':
    case 'S':
    case 'a':
    case 'A':
      size += sizeof(void*);
      break;
    default:
      return -(sym->type[i]);
    }
  }
  return size;
}

VALUE
rb_dlsym_call(int argc, VALUE argv[], VALUE self)
{
  struct sym_data *sym;
  ANY_TYPE *args;
  ANY_TYPE *dargs;
  ANY_TYPE ret;
  int   *dtypes;
  VALUE val;
  VALUE dvals;
  int i;
  long ftype;
  void *func;

  Data_Get_Struct(self, struct sym_data, sym);
  DEBUG_CODE({
    printf("rb_dlsym_call(): type = '%s', func = 0x%x\n", sym->type, sym->func);
  });
  if( (sym->len - 1) != argc ){
    rb_raise(rb_eArgError, "%d arguments are needed", sym->len - 1);
  };

  ftype = 0;
  dvals = Qnil;

  args = ALLOC_N(ANY_TYPE, sym->len - 1);
  dargs = ALLOC_N(ANY_TYPE, sym->len - 1);
  dtypes = ALLOC_N(int, sym->len - 1);
#define FREE_ARGS {xfree(args); xfree(dargs); xfree(dtypes);}

  for( i = sym->len - 2; i >= 0; i-- ){
    dtypes[i] = 0;

    switch( sym->type[i+1] ){
    case 'p':
      dtypes[i] = 'p';
    case 'P':
      {
	struct ptr_data *data;
	VALUE pval;

	if( argv[i] == Qnil ){
	  ANY2P(args[i]) = DLVOIDP(0);
	}
	else{
	  if( rb_obj_is_kind_of(argv[i], rb_cDLPtrData) ){
	    pval = argv[i];
	  }
	  else{
	    pval = rb_funcall(argv[i], rb_intern("to_ptr"), 0);
	    if( !rb_obj_is_kind_of(pval, rb_cDLPtrData) ){
	      rb_raise(rb_eDLTypeError, "unexpected type of argument #%d", i);
	    };
	  };
	  Data_Get_Struct(pval, struct ptr_data, data);
	  ANY2P(args[i]) = DLVOIDP(data->ptr);
	};
      };
      PUSH_P(ftype);
      break;
    case 'a':
      dtypes[i] = 'a';
    case 'A':
      if( argv[i] == Qnil ){
	ANY2P(args[i]) = DLVOIDP(0);
      }
      else{
	ANY2P(args[i]) = DLVOIDP(rb_ary2cary(0, argv[i], NULL));
      };
      PUSH_P(ftype);
      break;
    case 'C':
      ANY2C(args[i]) = DLCHAR(NUM2CHR(argv[i]));
      PUSH_C(ftype);
      break;
    case 'c':
      ANY2C(dargs[i]) = DLCHAR(NUM2CHR(argv[i]));
      ANY2P(args[i]) = DLVOIDP(&(ANY2C(dargs[i])));
      dtypes[i] = 'c';
      PUSH_P(ftype);
      break;
    case 'H':
      ANY2H(args[i]) = DLSHORT(NUM2CHR(argv[i]));
      PUSH_C(ftype);
      break;
    case 'h':
      ANY2H(dargs[i]) = DLSHORT(NUM2CHR(argv[i]));
      ANY2P(args[i]) = DLVOIDP(&(ANY2H(dargs[i])));
      dtypes[i] = 'h';
      PUSH_P(ftype);
      break;
    case 'I':
      ANY2I(args[i]) = DLINT(NUM2INT(argv[i]));
      PUSH_I(ftype);
      break;
    case 'i':
      ANY2I(dargs[i]) = DLINT(NUM2INT(argv[i]));
      ANY2P(args[i]) = DLVOIDP(&(ANY2I(dargs[i])));
      dtypes[i] = 'i';
      PUSH_P(ftype);
      break;
    case 'L':
      ANY2L(args[i]) = DLNUM2LONG(argv[i]);
      PUSH_L(ftype);
      break;
    case 'l':
      ANY2L(dargs[i]) = DLNUM2LONG(argv[i]);
      ANY2P(args[i]) = DLVOIDP(&(ANY2L(dargs[i])));
      dtypes[i] = 'l';
      PUSH_P(ftype);
      break;
    case 'F':
      Check_Type(argv[i], T_FLOAT);
      ANY2F(args[i]) = DLFLOAT(RFLOAT(argv[i])->value);
      PUSH_F(ftype);
      break;
    case 'f':
      Check_Type(argv[i], T_FLOAT);
      ANY2F(dargs[i]) = DLFLOAT(RFLOAT(argv[i])->value);
      ANY2P(args[i]) = DLVOIDP(&(ANY2F(dargs[i])));
      dtypes[i] = 'f';
      PUSH_P(ftype);
      break;
    case 'D':
      Check_Type(argv[i], T_FLOAT);
      ANY2D(args[i]) = RFLOAT(argv[i])->value;
      PUSH_D(ftype);
      break;
    case 'd':
      Check_Type(argv[i], T_FLOAT);
      ANY2D(dargs[i]) = RFLOAT(argv[i])->value;
      ANY2P(args[i]) = DLVOIDP(&(ANY2D(dargs[i])));
      dtypes[i] = 'd';
      PUSH_P(ftype);
      break;
    case 'S':
      if( argv[i] == Qnil ){
	ANY2S(args[i]) = DLSTR(0);
      }
      else{
	if( TYPE(argv[i]) != T_STRING ){
	  rb_raise(rb_eDLError, "#%d must be a string",i);
	};
	ANY2S(args[i]) = DLSTR(RSTRING(argv[i])->ptr);
      };
      PUSH_P(ftype);
      break;
    case 's':
      if( TYPE(argv[i]) != T_STRING ){
	rb_raise(rb_eDLError, "#%d must be a string",i);
      };
      ANY2S(args[i]) = DLSTR(dlmalloc(RSTRING(argv[i])->len + 1));
      memcpy((char*)(ANY2S(args[i])), RSTRING(argv[i])->ptr, RSTRING(argv[i])->len + 1);
      dtypes[i] = 's';
      PUSH_P(ftype);
      break;
    default:
      FREE_ARGS;
      rb_raise(rb_eDLTypeError,
	       "unknown type '%c' of the return value.",
	       sym->type[i+1]);
    };
  };

  switch( sym->type[0] ){
  case '0':
    PUSH_0(ftype);
    break;
  case 'P':
  case 'p':
  case 'S':
  case 's':
  case 'A':
  case 'a':
    PUSH_P(ftype);
    break;
  case 'C':
  case 'c':
    PUSH_C(ftype);
    break;
  case 'H':
  case 'h':
    PUSH_H(ftype);
    break;
  case 'I':
  case 'i':
    PUSH_I(ftype);
    break;
  case 'L':
  case 'l':
    PUSH_L(ftype);
    break;
  case 'F':
  case 'f':
    PUSH_F(ftype);
    break;
  case 'D':
  case 'd':
    PUSH_D(ftype);
    break;
  default:
    FREE_ARGS;
    rb_raise(rb_eDLTypeError,
	     "unknown type `%c' of the return value.",
	     sym->type[0]);
  };

  func = sym->func;

#if defined(DLSTACK)
  {
#if defined(DLSTACK_SIZE)
  int  stk_size;
  long stack[DLSTACK_SIZE];
  long *sp;

  sp = stack;
  stk_size = stack_size(sym);
  if( stk_size < 0 ){
    FREE_ARGS;
    rb_raise(rb_eDLTypeError, "unknown type '%c'.", -stk_size);
  }
  else if( stk_size > (int)(DLSTACK_SIZE) ){
    FREE_ARGS;
    rb_raise(rb_eArgError, "too many arguments.");
  }
#endif

  DLSTACK_START(sym);

#if defined(DLSTACK_REVERSE)
  for( i = sym->len - 2; i >= 0; i-- )
#else
  for( i = 0; i <= sym->len -2; i++ )
#endif
  {
    switch( sym->type[i+1] ){
    case 'p':
    case 'P':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'a':
    case 'A':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'C':
      DLSTACK_PUSH_C(ANY2C(args[i]));
      break;
    case 'c':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'H':
      DLSTACK_PUSH_H(ANY2H(args[i]));
      break;
    case 'h':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'I':
      DLSTACK_PUSH_I(ANY2I(args[i]));
      break;
    case 'i':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'L':
      DLSTACK_PUSH_L(ANY2L(args[i]));
      break;
    case 'l':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'F':
      DLSTACK_PUSH_F(ANY2F(args[i]));
      break;
    case 'f':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'D':
      DLSTACK_PUSH_D(ANY2D(args[i]));
      break;
    case 'd':
      DLSTACK_PUSH_P(ANY2P(args[i]));
      break;
    case 'S':
    case 's':
      DLSTACK_PUSH_P(ANY2S(args[i]));
      break;
    };
  }
  DLSTACK_END(sym->type);

  {
    switch( sym->type[0] ){
    case '0':
      {
	void (*f)(DLSTACK_PROTO) = func;
	f(DLSTACK_ARGS);
      };
      break;
    case 'P':
    case 'p':
      {
	void * (*f)(DLSTACK_PROTO) = func;
	ret.p = f(DLSTACK_ARGS);
      };
      break;
    case 'C':
    case 'c':
      {
	char (*f)(DLSTACK_PROTO) = func;
	ret.c = f(DLSTACK_ARGS);
      };
      break;
    case 'H':
    case 'h':
      {
	short (*f)(DLSTACK_PROTO) = func;
	ret.h = f(DLSTACK_ARGS);
      };
      break;
    case 'I':
    case 'i':
      {
	int (*f)(DLSTACK_PROTO) = func;
	ret.i = f(DLSTACK_ARGS);
      };
      break;
    case 'L':
    case 'l':
      {
	long (*f)(DLSTACK_PROTO) = func;
	ret.l = f(DLSTACK_ARGS);
      };
      break;
    case 'F':
    case 'f':
      {
	float (*f)(DLSTACK_PROTO) = func;
	ret.f = f(DLSTACK_ARGS);
      };
      break;
    case 'D':
    case 'd':
      {
	double (*f)(DLSTACK_PROTO) = func;
	ret.d = f(DLSTACK_ARGS);
      };
      break;
    case 'S':
    case 's':
      {
	char * (*f)(DLSTACK_PROTO) = func;
	ret.s = f(DLSTACK_ARGS);
      };
      break;
    default:
      FREE_ARGS;
      rb_raise(rb_eDLTypeError, "unknown type `%c'", sym->type[0]);
    }
  }
  }
#else /* defined(DLSTACK) */
  switch(ftype){
#include "call.func"
  default:
    FREE_ARGS;
    rb_raise(rb_eDLTypeError, "unsupported function type `%s'", sym->type);
  };
#endif /* defined(DLSTACK) */

  switch( sym->type[0] ){
  case '0':
    val = Qnil;
    break;
  case 'P':
    val = rb_dlptr_new((void*)(ANY2P(ret)), 0, 0);
    break;
  case 'p':
    val = rb_dlptr_new((void*)(ANY2P(ret)), 0, dlfree);
    break;
  case 'C':
  case 'c':
    val = CHR2FIX((char)(ANY2C(ret)));
    break;
  case 'H':
  case 'h':
    val = INT2NUM((short)(ANY2H(ret)));
    break;
  case 'I':
  case 'i':
    val = INT2NUM((int)(ANY2I(ret)));
    break;
  case 'L':
  case 'l':
    val = DLLONG2NUM((long)(ANY2L(ret)));
    break;
  case 'F':
  case 'f':
    val = rb_float_new((double)(ANY2F(ret)));
    break;
  case 'D':
  case 'd':
    val = rb_float_new((double)(ANY2D(ret)));
    break;
  case 'S':
    if( ANY2S(ret) ){
      val = rb_tainted_str_new2((char*)(ANY2S(ret)));
    }
    else{
      val = Qnil;
    };
    break;
  case 's':
    if( ANY2S(ret) ){
      val = rb_tainted_str_new2((char*)(ANY2S(ret)));
      DEBUG_CODE({
	printf("dlfree(%s)\n",(char*)(ANY2S(ret)));
      });
      dlfree((void*)(ANY2S(ret)));
    }
    else{
      val = Qnil;
    };
    break;
  default:
    FREE_ARGS;
    rb_raise(rb_eDLTypeError, "unknown type `%c'", sym->type[0]);
  };

  dvals = rb_ary_new();
  for( i = 0; i <= sym->len - 2; i++ ){
    if( dtypes[i] ){
      switch( dtypes[i] ){
      case 'c':
	rb_ary_push(dvals, CHR2FIX(*((char*)(ANY2P(args[i])))));
	break;
      case 'h':
	rb_ary_push(dvals, INT2NUM(*((short*)(ANY2P(args[i])))));
	break;
      case 'i':
	rb_ary_push(dvals, INT2NUM(*((int*)(ANY2P(args[i])))));
	break;
      case 'l':
        rb_ary_push(dvals, DLLONG2NUM(*((long*)(ANY2P(args[i])))));
        break;
      case 'f':
	rb_ary_push(dvals, rb_float_new(*((float*)(ANY2P(args[i])))));
	break;
      case 'd':
	rb_ary_push(dvals, rb_float_new(*((double*)(ANY2P(args[i])))));
	break;
      case 'p':
	rb_ary_push(dvals, rb_dlptr_new((void*)(ANY2P(args[i])), 0, 0));
	break;
      case 'a':
	rb_ary_push(dvals, rb_dlptr_new((void*)ANY2P(args[i]), 0, 0));
	break;
      case 's':
	rb_ary_push(dvals, rb_tainted_str_new2((char*)ANY2S(args[i])));
	DEBUG_CODE({
	  printf("dlfree(%s)\n",(char*)ANY2S(args[i]));
	});
	dlfree((void*)ANY2S(args[i]));
	break;
      default:
	{
	  char c = dtypes[i];
	  FREE_ARGS;
	  rb_raise(rb_eRuntimeError, "unknown argument type '%c'", i, c);
	};
      };
    }
    else{
      switch( sym->type[i+1] ){
      case 'A':
	dlfree((void*)ANY2P(args[i]));
	break;
      };
      rb_ary_push(dvals, argv[i]);
    };
  };

#undef FREE_ARGS
  return rb_assoc_new(val,dvals);
}

VALUE
rb_dlsym_to_i(VALUE self)
{
  struct sym_data *sym;

  Data_Get_Struct(self, struct sym_data, sym);
  return DLLONG2NUM(sym);
}

VALUE
rb_dlsym_to_ptr(VALUE self)
{
  struct sym_data *sym;

  Data_Get_Struct(self, struct sym_data, sym);
  return rb_dlptr_new(sym->func, sizeof(freefunc_t), 0);
}

void
Init_dlsym()
{
  rb_cDLSymbol = rb_define_class_under(rb_mDL, "Symbol", rb_cObject);
  rb_define_singleton_method(rb_cDLSymbol, "allocate", rb_dlsym_s_allocate, 0);
  rb_define_singleton_method(rb_cDLSymbol, "char2type", rb_s_dlsym_char2type, 1);
  rb_define_method(rb_cDLSymbol, "initialize", rb_dlsym_initialize, -1);
  rb_define_method(rb_cDLSymbol, "call", rb_dlsym_call, -1);
  rb_define_method(rb_cDLSymbol, "[]",   rb_dlsym_call, -1);
  rb_define_method(rb_cDLSymbol, "name", rb_dlsym_name, 0);
  rb_define_method(rb_cDLSymbol, "proto", rb_dlsym_proto, 0);
  rb_define_method(rb_cDLSymbol, "cproto", rb_dlsym_cproto, 0);
  rb_define_method(rb_cDLSymbol, "inspect", rb_dlsym_inspect, 0);
  rb_define_method(rb_cDLSymbol, "to_s", rb_dlsym_cproto, 0);
  rb_define_method(rb_cDLSymbol, "to_ptr", rb_dlsym_to_ptr, 0);
  rb_define_method(rb_cDLSymbol, "to_i", rb_dlsym_to_i, 0);
}
