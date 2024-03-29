/**********************************************************************

  rubysig.h -

  $Author$
  $Date$
  created at: Wed Aug 16 01:15:38 JST 1995

  Copyright (C) 1993-2002 Yukihiro Matsumoto

**********************************************************************/

#ifndef SIG_H
#define SIG_H

#ifdef NT
typedef LONG rb_atomic_t;

# define ATOMIC_TEST(var) InterlockedExchange(&(var), 0)
# define ATOMIC_SET(var, val) InterlockedExchange(&(var), (val))
# define ATOMIC_INC(var) InterlockedIncrement(&(var))
# define ATOMIC_DEC(var) InterlockedDecrement(&(var))

/* Windows doesn't allow interrupt while system calls */
# define TRAP_BEG do {\
    rb_atomic_t trap_immediate = ATOMIC_SET(rb_trap_immediate, 1)
# define TRAP_END\
	ATOMIC_SET(rb_trap_immediate, trap_immediate);\
} while (0)
# define RUBY_CRITICAL(statements) do {\
    rb_w32_enter_critical();\
    statements;\
    rb_w32_leave_critical();\
} while (0)
#else
typedef int rb_atomic_t;

# define ATOMIC_TEST(var) ((var) ? ((var) = 0, 1) : 0)
# define ATOMIC_SET(var, val) ((var) = (val))
# define ATOMIC_INC(var) (++(var))
# define ATOMIC_DEC(var) (--(var))

# define TRAP_BEG do {\
    int trap_immediate = rb_trap_immediate;\
    rb_trap_immediate = 1
# define TRAP_END rb_trap_immediate = trap_immediate;\
} while (0)

# define RUBY_CRITICAL(statements) do {\
    int trap_immediate = rb_trap_immediate;\
    rb_trap_immediate = 0;\
    statements;\
    rb_trap_immediate = trap_immediate;\
} while (0)
#endif
EXTERN rb_atomic_t rb_trap_immediate;

EXTERN int rb_prohibit_interrupt;
#define DEFER_INTS (rb_prohibit_interrupt++)
#define ALLOW_INTS do {\
    rb_prohibit_interrupt--;\
    CHECK_INTS;\
} while (0)
#define ENABLE_INTS (rb_prohibit_interrupt--)

VALUE rb_with_disable_interrupt _((VALUE(*)(ANYARGS),VALUE));

EXTERN rb_atomic_t rb_trap_pending;
void rb_trap_restore_mask _((void));

EXTERN int rb_thread_critical;
void rb_thread_schedule _((void));
#if defined(HAVE_SETITIMER) && !defined(__BOW__)
EXTERN int rb_thread_pending;
# define CHECK_INTS do {\
    if (!rb_prohibit_interrupt) {\
	if (rb_trap_pending) rb_trap_exec();\
	if (rb_thread_pending && !rb_thread_critical)\
	    rb_thread_schedule();\
    }\
} while (0)
#else
/* pseudo preemptive thread switching */
EXTERN int rb_thread_tick;
#define THREAD_TICK 500
#define CHECK_INTS do {\
    if (!rb_prohibit_interrupt) {\
	if (rb_trap_pending) rb_trap_exec();\
	if (!rb_thread_critical) {\
	    if (rb_thread_tick-- <= 0) {\
		rb_thread_tick = THREAD_TICK;\
		rb_thread_schedule();\
	    }\
	}\
    }\
} while (0)
#endif

#endif
