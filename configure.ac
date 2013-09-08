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

dnl Tested with this version of Autoconf; older might be feasible.
AC_PREREQ([2.63])

# Preliminaries.
AC_INIT([ottery], [0.0.0-dev])
AC_CONFIG_SRCDIR([src/ottery.c])
AC_CONFIG_MACRO_DIR([m4])

# config.status creates these files.
AC_CONFIG_FILES([Makefile libottery.pc])
AC_CONFIG_HEADERS([src/ottery-config.h])

# Automake initialization.
AM_INIT_AUTOMAKE

# AM_SILENT_RULES was added in automake 1.11.
# Silent rules are off by default, but can be enabled at configure time
# with --enable-silent-rules, or at make time with "make V=0".
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

# Configuration options.
# TODO: Remove some of these.
AC_ARG_ENABLE(pid-check,
     AS_HELP_STRING(--disable-pid-check,
         [Disable checks to see if the process has forked. Dangerous!]))
if test "$enable_pid_check" = no; then
   AC_DEFINE(OTTERY_NO_PID_CHECK, 1, [If defined, disable PID checking. (DANGER!)])
fi

AC_ARG_ENABLE(init-check,
     AS_HELP_STRING(--disable-init-check,
         [Disable checks to make sure that the PRNG is initialized. Dangerous!]))
if test "$enable_init_check" = no; then
   AC_DEFINE(OTTERY_NO_INIT_CHECK, 1, [If defined, disable initialization checking. (DANGER!)])
fi

AC_ARG_ENABLE(simd,
     AS_HELP_STRING(--disable-simd,
         [Don't build with support for fast crypto using SIMD]), ,
	 enable_simd=yes)
if test "$enable_simd" = no; then
   AC_DEFINE(OTTERY_NO_VECS, 1, [If defined, disable SIMD crypto])
fi

AC_ARG_ENABLE(locks,
     AS_HELP_STRING(--disable-locks,
         [Don't build with locking support]), , enable_locks=yes)
if test "$enable_locks" = no; then
   AC_DEFINE(OTTERY_NO_LOCKS, 1, [If defined, disable locking])
fi

AC_ARG_ENABLE(spinlocks,
     AS_HELP_STRING(--disable-spinlocks,
         [When using locking, don't use spinlocks]), , enable_spinlocks=yes)
if test "$enable_spinlocks" = no; then
   AC_DEFINE(OTTERY_NO_SPINLOCKS, 1, [If defined, don't use spinlocks for locking])
fi

AC_ARG_ENABLE(clear-after-yield,
     AS_HELP_STRING(--disable-clear-after-yield,
         [Don't clear data from our buffer immediately after yielding it]), ,
	 enable_clear_after_yield=yes)
if test "$enable_clear_after_yield" = no; then
   AC_DEFINE(OTTERY_NO_CLEAR_AFTER_YIELD, 1, [If defined, don't clear the buffer after producing output.])
fi

AC_ARG_ENABLE(wipe-stack,
     AS_HELP_STRING(--disable-wipe-stack,
         [Don't attempt to wipe the stack after calling the crypto core]), ,
	 enable_wipe_stack=yes)
if test "$enable_wipe_stack" = no; then
   AC_DEFINE(OTTERY_NO_WIPE_STACK, 1, [If defined, don't try to wipe the stack after calling the crypto core.])
fi

#
# C compiler configuration.
#

AC_PROG_CC
dnl This is necessary if using automake older than 1.14.
AM_PROG_CC_C_O
dnl This has to appear before any compilation checks.
dnl AC_USE_SYSTEM_EXTENSIONS is reliably available since autoconf 2.59.
AC_USE_SYSTEM_EXTENSIONS

#
# C environment.
#

# On x86 (-32 or -64), if the compiler supports both SSE2-only and
# SSE2+SSSE3 compilation modes, we want to compile chacha_krovetz.c
# both ways and select which one is best for the CPU at runtime.

AC_CACHE_CHECK([for SSE2 intrinsics], [ac_cv_cpu_x86_sse2_intrinsics],
  [ac_cv_cpu_x86_sse2_intrinsics=no
  save_CFLAGS="$CFLAGS"
  for opts in "-msse2 -mno-sse3" ; do
    CFLAGS="$save_CFLAGS $opts"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !__SSE2__
#error "SSE2 test macro should be defined"
#endif
#if __SSSE3__
#error "SSSE3 test macro should not be defined"
#endif
#include <emmintrin.h>
]], [[
 extern __m128i x;
 x = _mm_set_epi8(0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1);
]])],
      [ac_cv_cpu_x86_sse2_intrinsics="$opts"; break])
  done
  CFLAGS="$save_CFLAGS"
])
if test x"$ac_cv_cpu_x86_sse2_intrinsics" = xno; then
  SSE2_CFLAGS=
else
  SSE2_CFLAGS="$ac_cv_cpu_x86_sse2_intrinsics"
fi
AC_SUBST(SSE2_CFLAGS)

AC_CACHE_CHECK([for SSSE3 intrinsics], [ac_cv_cpu_x86_ssse3_intrinsics],
  [ac_cv_cpu_x86_ssse3_intrinsics=no
  save_CFLAGS="$CFLAGS"
  for opts in "-mssse3" ; do
    CFLAGS="$save_CFLAGS $opts"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !__SSE2__
#error "SSE2 test macro should be defined"
#endif
#if !__SSSE3__
#error "SSSE3 test macro should be defined"
#endif
#include <emmintrin.h>
#include <tmmintrin.h>
]], [[
 extern __m128i x, y;
 y = _mm_shuffle_epi8((__m128i)x,
         _mm_set_epi8(14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3));
]])],
      [ac_cv_cpu_x86_ssse3_intrinsics="$opts"; break])
  done
  CFLAGS="$save_CFLAGS"
])
if test x"$ac_cv_cpu_x86_ssse3_intrinsics" = xno; then
  SSSE3_CFLAGS=
else
  SSSE3_CFLAGS="$ac_cv_cpu_x86_ssse3_intrinsics"
fi
AC_SUBST(SSSE3_CFLAGS)

AM_CONDITIONAL(CHACHA_KROVETZ_X86_TWO_FLAVORS,
  [test x"$SSE2_CFLAGS" != x && test x"$SSSE3_CFLAGS" != x])

# Initialize libtool.  Must be done after the compiler is set up.

dnl Some systems still ship a libtool.m4 that predates the change to LT_INIT.
AC_PROG_LIBTOOL

# Python and Haskell are used in the test suite.
# Do these checks last so the warnings don't scroll off the user's terminal.

AC_ARG_VAR(PYTHON)
AC_CHECK_PROGS(PYTHON, [python python2 python3])
if test "x$PYTHON" = "x"; then
  AC_MSG_WARN([Python unavailable; some tests will not be run.])
fi
AM_CONDITIONAL(USEPYTHON, [test "x$PYTHON" != "x"])

AC_ARG_VAR(GHC)
AC_SUBST(GHC_MTL)
GHC_MTL=no
AC_CHECK_PROGS(GHC, [ghc])
if test "x$GHC" = "x"; then
  AC_MSG_WARN([GHC unavailable; some tests will not be run.])
else
  # In addition to the base Haskell compiler, the "Control.Monad.State"
  # library is required (from the mtl package).
  AC_CACHE_CHECK([for Control.Monad.State], ac_cv_ghc_control_monad_state, [
    cat <<\EOF > conftest.hs
import Control.Monad.State
main = undefined
EOF
    $GHC -c conftest.hs 1>&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD
    if test $? -eq 0; then
      ac_cv_ghc_control_monad_state=yes
    else
      ac_cv_ghc_control_monad_state=no
    fi
    rm -f conftest.*
  ])
  if test $ac_cv_ghc_control_monad_state = yes; then
    GHC_MTL=yes
  else
    AC_MSG_WARN([Control.Monad.State unavailable; some tests will not be run.])
    AC_MSG_WARN([Control.Monad.State is in the Haskell package 'mtl'.])
  fi
fi
AM_CONDITIONAL(USEGHC, [test "x$GHC" != "x" && test "$GHC_MTL" = "yes"])

# Done.
AC_OUTPUT
