#  Libottery by Nick Mathewson.
#
#  This software has been dedicated to the public domain under the CC0
#  public domain dedication.
#
#  To the extent possible under law, the person who associated CC0 with
#  libottery has waived all copyright and related or neighboring rights
#  to libottery.
#
#  You should have received a copy of the CC0 legalcode along with this
#  work in doc/cc0.txt.  If not, see
#  <http://creativecommons.org/publicdomain/zero/1.0/>.

dnl Set up autoconf, and define the version number.
AC_INIT(ottery, 0.0.0-dev)

dnl We must have autoconf 2.59 or later.
dnl TODO: Actually test with autoconf 2.59 to see if it works.
AC_PREREQ(2.59)

dnl Autoconf wants to know that the source actually is where we expect,
dnl in case somebody overrode the default with --srcddir=
AC_CONFIG_SRCDIR(src/ottery.c)

dnl Look for extra macro definitions in the m4/ directory
AC_CONFIG_MACRO_DIR([m4])

dnl We're going to be using automake.
AM_INIT_AUTOMAKE

dnl We'd like to use AM_SILENT_RULES if it's possible, for a nice
dnl quiet build, but not every automake has it.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Now process all of the --disable-foo options that we support.  Each has
dnl a corresponding macro in ottery-config.h
dnl TODO: Remove some of these.
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

dnl See whether we have python and ghc; we use them in some of our tests
dnl if they're available.
AC_ARG_VAR(PYTHON)
AC_CHECK_PROGS(PYTHON, [python python2 python3])
AM_CONDITIONAL(USEPYTHON, [test "x$PYTHON" != "x"])

AC_ARG_VAR(GHC)
AC_CHECK_PROGS(GHC,    [ghc])
AM_CONDITIONAL(USEGHC,    [test "x$GHC"    != "x"])

dnl Try and get a full POSIX environment on systems where you need magic to
dnl get it.  The AC_USE_SYSTEM_EXTENSIONS macro is best, but older autoconfs
dnl don't have it.
ifdef([AC_USE_SYSTEM_EXTENSIONS], [
AC_USE_SYSTEM_EXTENSIONS
], [
AC_AIX
AC_GNU_SOURCE
AC_Write
])

dnl MINIX our C macros to src/ottery-config.h.  This is an internal-use-only
dnl file that we won't install.
AC_CONFIG_HEADERS(src/ottery-config.h)

dnl Make sure that we detect our platform and target platform in their
dnl canonical form.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

dnl Checks for programs we need to build and install
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S

dnl Work around old broken GCCs that needed the -traditional flag to work
dnl properly.
dnl TODO: Identify which versions and decide whether we care.
AC_PROG_GCC_TRADITIONAL

dnl We use libtool for building shared libraries.
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

dnl Our build setup is a bit different if we're on an X86 or X86_64, since
dnl we'll be building separate sse2 and sse3 versions of the simd code.
dnl We'll detect now what the compiler thinks we're building for.
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

dnl Generate a pkgconfig file.
AC_CONFIG_FILES( [libottery.pc] )

dnl Generate the main makefile.
AC_OUTPUT(Makefile)

