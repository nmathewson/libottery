
AC_INIT(ottery, 0.0.0-dev)
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR(src/ottery.c)

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
dnl AM_SILENT_RULES req. automake 1.11.  [no] defaults V=1
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_DEFINE(NUMERIC_VERSION, 0x00000000, [Numeric representation of the version])


AC_ARG_ENABLE(pid-check,
     AS_HELP_STRING(--disable-pid-check,
         [Disable checks to see if the process has forked. Dangerous!]))
AC_ARG_ENABLE(init-check,
     AS_HELP_STRING(--disable-init-check,
         [Disable checks to make sure that the PRNG is initialized. Dangerous!]))
AC_ARG_ENABLE(simd,
     AS_HELP_STRING(--disable-simd,
         [Don't build with support for fast crypto using SIMD]), ,
	 enable_simd=yes)
AC_ARG_ENABLE(locks,
     AS_HELP_STRING(--disable-locks,
         [Don't build with locking support]), , enable_locks=yes)
AC_ARG_ENABLE(spinlocks,
     AS_HELP_STRING(--disable-spinlocks,
         [When using locking, don't use spinlocks]]), , enable_spinlocks=yes)
AC_ARG_ENABLE(clear-after-yield,
     AS_HELP_STRING(--disable-clear-after-yield,
         [Don't clear data from our buffer immediately after yielding it]), ,
	 enable_clear_after_yield=yes)
AC_ARG_ENABLE(wipe-stack,
     AS_HELP_STRING(--disable-wipe-stack,
         [Don't attempt to wipe the stack after calling the crypto core]), ,
	 enable_wipe_stack=yes)

if test "$enable_pid_check" = no; then
   AC_DEFINE(OTTERY_NO_PID_CHECK, 1, [If defined, disable PID checking. (DANGER!)])
fi
if test "$enable_init_check" = no; then
   AC_DEFINE(OTTERY_NO_INIT_CHECK, 1, [If defined, disable initialization checking. (DANGER!)])
fi
if test "$enable_simd" = no; then
   AC_DEFINE(OTTERY_NO_VECS, 1, [If defined, disable SIMD crypto])
fi
if test "$enable_locks" = no; then
   AC_DEFINE(OTTERY_NO_LOCKS, 1, [If defined, disable locking])
fi
if test "$enable_spinlocks" = no; then
   AC_DEFINE(OTTERY_NO_SPINLOCKS, 1, [If defined, don't use spinlocks for locking])
fi
if test "$enable_clear_after_yield" = no; then
   AC_DEFINE(OTTERY_NO_CLEAR_AFTER_YIELD, 1, [If defined, don't clear the buffer after producing output.])
fi
if test "$enable_wipe_stack" = no; then
   AC_DEFINE(OTTERY_NO_WIPE_STACK, 1, [If defined, don't try to wipe the stack after calling the crypto core.])
fi



dnl Try and get a full POSIX environment on obscure systems
ifdef([AC_USE_SYSTEM_EXTENSIONS], [
AC_USE_SYSTEM_EXTENSIONS
], [
AC_AIX
AC_GNU_SOURCE
AC_MINIX
])

AC_CONFIG_HEADERS(src/ottery-config.h)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

dnl Checks for programs.
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S

AC_PROG_GCC_TRADITIONAL

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_MSG_CHECKING([whether CPU is X86 or X86_64])
AC_TRY_COMPILE(,
	[
#if defined(i386) || \
    defined(__i386) || \
    defined(__x86_64) || \
    defined(__M_IX86) || \
    defined(_M_IX86) || \
    defined(__INTEL_COMPILER)
#else
die horribly
#endif
	],
	is_x86=yes; AC_MSG_RESULT(yes),
	is_x86=no; AC_MSG_RESULT(no),
)
AM_CONDITIONAL(X86, [test "$is_x86" = "yes"])

AC_CONFIG_FILES( [libottery.pc] )
AC_OUTPUT(Makefile)

