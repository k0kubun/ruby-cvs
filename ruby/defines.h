/************************************************

  defines.h -

  $Author$
  $Date$
  created at: Wed May 18 00:21:44 JST 1994

************************************************/
#ifndef DEFINES_H
#define DEFINES_H

#define RUBY

/* define RUBY_USE_EUC/SJIS for default kanji-code */
#if defined(MSDOS) || defined(__CYGWIN32__) || defined(__human68k__) || defined(__MACOS__)
#undef RUBY_USE_EUC
#define RUBY_USE_SJIS
#else
#define RUBY_USE_EUC
#undef RUBY_USE_SJIS
#endif

#ifdef NeXT
#define DYNAMIC_ENDIAN		/* determine endian at runtime */
#ifndef __APPLE__
#define S_IXUSR _S_IXUSR        /* execute/search permission, owner */
#endif
#define S_IXGRP 0000010         /* execute/search permission, group */
#define S_IXOTH 0000001         /* execute/search permission, other */
#endif /* NeXT */

#ifdef NT
#include "missing/nt.h"
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef sparc
#define FLUSH_REGISTER_WINDOWS asm("ta 3")
#else
#define FLUSH_REGISTER_WINDOWS /* empty */
#endif

#if defined(MSDOS) || defined(NT) || defined(__human68k__) || defined(__MACOS__)
#define RUBY_PATH_SEP ";"
#else
#define RUBY_PATH_SEP ":"
#endif

#if defined(__human68k__) || defined(__CYGWIN32__)
#undef HAVE_RANDOM
#undef HAVE_SETITIMER
#endif

#ifndef RUBY_PLATFORM
#define RUBY_PLATFORM "unknown-unknown"
#endif

#endif
