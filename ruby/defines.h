/************************************************

  defines.h -

  $Author$
  $Date$
  created at: Wed May 18 00:21:44 JST 1994

************************************************/
#ifndef DEFINES_H
#define DEFINES_H

#define RUBY

#if !defined(__STDC__) && !defined(_MSC_VER)
# define volatile
#endif

#ifdef __cplusplus
# ifndef  HAVE_PROTOTYPES
#  define HAVE_PROTOTYPES 1
# endif
# ifndef  HAVE_STDARG_PROTOTYPES
#  define HAVE_STDARG_PROTOTYPES 1
# endif
#endif

#undef _
#ifdef HAVE_PROTOTYPES
# define _(args) args
#else
# define _(args) ()
#endif

#undef __
#ifdef HAVE_STDARG_PROTOTYPES
# define __(args) args
#else
# define __(args) ()
#endif

#ifdef __cplusplus
#define ANYARGS ...
#else
#define ANYARGS
#endif

#define xmalloc ruby_xmalloc
#define xcalloc ruby_xcalloc
#define xrealloc ruby_xrealloc
#define xfree ruby_xfree

void *xmalloc _((long));
void *xcalloc _((long,long));
void *xrealloc _((void*,long));
void xfree _((void*));

#if SIZEOF_LONG_LONG > 0
# define LONG_LONG long long
#elif SIZEOF___INT64 > 0
# define HAVE_LONG_LONG 1
# define LONG_LONG __int64
# undef SIZEOF_LONG_LONG
# define SIZEOF_LONG_LONG SIZEOF___INT64
#endif

#if SIZEOF_INT*2 <= SIZEOF_LONG_LONG
# define BDIGIT unsigned int
# define SIZEOF_BDIGITS SIZEOF_INT
# define BDIGIT_DBL unsigned LONG_LONG
# define BDIGIT_DBL_SIGNED LONG_LONG
#elif SIZEOF_INT*2 <= SIZEOF_LONG
# define BDIGIT unsigned int
# define SIZEOF_BDIGITS SIZEOF_INT
# define BDIGIT_DBL unsigned long
# define BDIGIT_DBL_SIGNED long
#else
# define BDIGIT unsigned short
# define SIZEOF_BDIGITS SIZEOF_SHORT
# define BDIGIT_DBL unsigned long
# define BDIGIT_DBL_SIGNED long
#endif

/* define RUBY_USE_EUC/SJIS for default kanji-code */
#ifndef DEFAULT_KCODE
#if defined(MSDOS) || defined(__CYGWIN__) || defined(__human68k__) || defined(__MACOS__) || defined(__EMX__) || defined(OS2) || defined(NT)
#define DEFAULT_KCODE KCODE_SJIS
#else
#define DEFAULT_KCODE KCODE_EUC
#endif
#endif

#ifdef NeXT
#define DYNAMIC_ENDIAN		/* determine endian at runtime */
#ifndef __APPLE__
#define S_IXUSR _S_IXUSR        /* execute/search permission, owner */
#endif
#define S_IXGRP 0000010         /* execute/search permission, group */
#define S_IXOTH 0000001         /* execute/search permission, other */

#define HAVE_SYS_WAIT_H         /* configure fails to find this */
#endif /* NeXT */

#ifdef NT
#include "win32/win32.h"
#endif

#if defined(__VMS)
#include "vms/vms.h"
#endif

#if defined __CYGWIN__
# undef EXTERN
# if defined USEIMPORTLIB
#  define EXTERN extern __declspec(dllimport)
# else
#  define EXTERN extern __declspec(dllexport)
# endif
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#if defined(sparc) || defined(__sparc__)
# if defined(linux) || defined(__linux__)
#define FLUSH_REGISTER_WINDOWS  asm("ta  0x83")
# else /* Solaris, not sparc linux */
#define FLUSH_REGISTER_WINDOWS  asm("ta  0x03")
# endif /* trap always to flush register windows if we are on a Sparc system */
#else /* Not a sparc, so */
#define FLUSH_REGISTER_WINDOWS  /* empty -- nothing to do here */
#endif 

#if defined(MSDOS) || defined(_WIN32) || defined(__human68k__) || defined(__EMX__)
#define DOSISH 1
#endif

#if defined(MSDOS) || defined(NT) || defined(__human68k__) || defined(OS2)
#define PATH_SEP ";"
#elif defined(riscos)
#define PATH_SEP ","
#else
#define PATH_SEP ":"
#endif
#define PATH_SEP_CHAR PATH_SEP[0]

#if defined(__human68k__)
#undef HAVE_RANDOM
#undef HAVE_SETITIMER
#endif

#if defined(DJGPP) || defined(__BOW__)
#undef HAVE_SETITIMER
#endif

#ifndef RUBY_PLATFORM
#define RUBY_PLATFORM "unknown-unknown"
#endif

#endif
