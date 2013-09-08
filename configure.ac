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
dnl In addition to the base Haskell compiler, the "Control.Monad.State"
dnl library is required (from the mtl package).
GHC_MTL=no
if test "x$GHC" != "x"; then
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
AC_SUBST(GHC_MTL)
AM_CONDITIONAL(USEGHC, [test "x$GHC" != "x" && test "$GHC_MTL" = "yes"])

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

dnl Generate a pkgconfig file.
AC_CONFIG_FILES( [libottery.pc] )

dnl Generate the main makefile.
AC_OUTPUT(Makefile)

