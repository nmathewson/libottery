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

# Macros specific to libottery configuration.

# Alias for m4_map_args_sep on platforms that lack it. Definition taken
# straight from autoconf
AC_DEFUN([ottery_map_args_sep],
[m4_if([$#], [0], [m4_fatal([$0: too few arguments: $#])],
       [$#], [1], [],
       [$#], [2], [],
       [$#], [3], [],
       [$#], [4], [$1[$4]$2[]],
       [$1[$4]$2[]_m4_foreach([$3[]$1], [$2], m4_shift3($@))])])

# Factoring out boilerplate for configuration-time options:
AC_DEFUN([OTTERY_ARG_DISABLE],
[_OTTERY_ARG_DISABLE([$1],
                     m4_translit([$1], [-], [_]),
                     m4_translit([$1], [-a-z], [_A-Z]),
                     [$2])])

# Internal macro which must be called as follows:
# _OTTERY_ARG_DISABLE([thing-one], [thing_one], [THING_ONE], [help text])
AC_DEFUN([_OTTERY_ARG_DISABLE],
[AC_ARG_ENABLE([$1], [AS_HELP_STRING([--disable-$1], [disable $4])],
               [], [enable_$2=yes])
if test x"$[]enable_$2" = xno; then
  AC_DEFINE([OTTERY_NO_$3], [1], [If defined to 1, disables $4])
fi])

# Probing compiler and host CPU for SIMD intrinsics.

# Internal: test for a specific type of SIMD.
# _OTTERY_CHECK_SIMD_SPECIFIC([message], [cache-var-without-ac_cv_cpu_],
#  [output_variable],
#  [comma-separated list of compiler options],
#  [[code]], [[more code]])
# [[code]] and [[more code]] will be passed to AC_LANG_PROGRAM.
AC_DEFUN([_OTTERY_CHECK_SIMD_SPECIFIC],
[AC_CACHE_CHECK([for $1], [ac_cv_cpu_$2],
  [ac_cv_cpu_$2=no
  save_CFLAGS="$CFLAGS"
  for opts in ottery_map_args_sep(["], ["], [ ], $4); do
    CFLAGS="$save_CFLAGS $opts"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$5], [$6])],
      [ac_cv_cpu_$2="$opts"
       break])
  done
  CFLAGS="$save_CFLAGS"
])
if test x"$[]ac_cv_cpu_$2" = xno; then
  $3=
else
  $3="$[]ac_cv_cpu_$2"
fi
])

# Each actual probe is quite verbose, so we wrap them up in macros to make
# the case statement below more readable.

# x86
AC_DEFUN([_OTTERY_CHECK_SIMD_SSE2],
[_OTTERY_CHECK_SIMD_SPECIFIC([SSE2 intrinsics],
  [x86_sse2_intrinsics], [SIMD1_CFLAGS], [-msse2 -mno-sse3], [[
#if !__SSE2__
#error "SSE2 test macro should be defined"
#endif
#if __SSSE3__
#error "SSSE3 test macro should not be defined"
#endif
#include <emmintrin.h>
]], [[
 typedef unsigned vec __attribute__ ((vector_size (16)));
 extern vec x;
 x = (vec)_mm_set_epi8(0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1);
]])])

AC_DEFUN([_OTTERY_CHECK_SIMD_SSSE3],
[_OTTERY_CHECK_SIMD_SPECIFIC([SSSE3 intrinsics],
  [x86_ssse3_intrinsics], [SIMD2_CFLAGS], [-mssse3], [[
#if !__SSE2__
#error "SSE2 test macro should be defined"
#endif
#if !__SSSE3__
#error "SSSE3 test macro should be defined"
#endif
#include <emmintrin.h>
#include <tmmintrin.h>
]], [[
 typedef unsigned vec __attribute__ ((vector_size (16)));
 extern vec x, y;
 y = (vec)_mm_shuffle_epi8((__m128i)x,
            _mm_set_epi8(14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3));
]])])

# ARM

AC_DEFUN([_OTTERY_CHECK_SIMD_NEON],
[_OTTERY_CHECK_SIMD_SPECIFIC([NEON intrinsics],
  [arm_neon_intrinsics], [SIMD1_CFLAGS], [-mfpu=neon], [[
#if !__ARM_NEON__
#error "NEON test macro should be defined"
#endif
#include <arm_neon.h>
]], [[
  typedef unsigned vec __attribute__ ((vector_size (16)));
  extern vec v, x;
  v = (vec)vextq_u32((uint32x4_t)x,(uint32x4_t)x,1);
]])])

# PPC

AC_DEFUN([_OTTERY_CHECK_SIMD_ALTIVEC],
[_OTTERY_CHECK_SIMD_SPECIFIC([AltiVec intrinsics],
  [ppc_altivec_intrinsics], [SIMD1_CFLAGS], [-maltivec], [[
#if !__ALTIVEC__
#error "AltiVec test macro should be defined"
#endif
#include <altivec.h>
]], [[
  typedef unsigned vec __attribute__ ((vector_size (16)));
  extern vec v, x;
  v = vec_perm(x,x,(vector char){3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12});
]])])

# Determine the host CPU and test for the appropriate set of intrinsics.

AC_DEFUN([OTTERY_USE_SIMD],
[AC_REQUIRE([AC_CANONICAL_HOST])
SIMD1_CFLAGS=
SIMD2_CFLAGS=
AS_IF([test $enable_simd = yes],
  [AS_CASE([$host_cpu],
  [i?86 | x86_64], [
    _OTTERY_CHECK_SIMD_SSE2
    _OTTERY_CHECK_SIMD_SSSE3
  ],
  [arm*], [
    _OTTERY_CHECK_SIMD_NEON
  ],
  [powerpc* | rs6000], [
    _OTTERY_CHECK_SIMD_ALTIVEC
  ],
  [*], [
  ])])
AS_IF([test x"$SIMD1_CFLAGS" = x && x"$SIMD2_CFLAGS" != x],
  [SIMD1_CFLAGS="$SIMD2_CFLAGS"
  SIMD2_CFLAGS=])
AC_SUBST(SIMD1_CFLAGS)dnl
AC_SUBST(SIMD2_CFLAGS)dnl
AM_CONDITIONAL(SIMD_CHACHA_1, [test x"$SIMD1_CFLAGS" != x])
AS_IF([test x"$SIMD1_CFLAGS" != x],
  [AC_DEFINE([HAVE_SIMD_CHACHA], [1],
    [Define to 1 if a SIMD-optimized ChaCha implementation is available.])])
AM_CONDITIONAL(SIMD_CHACHA_2, [test x"$SIMD2_CFLAGS" != x])
AS_IF([test x"$SIMD2_CFLAGS" != x],
  [AC_DEFINE([HAVE_SIMD_CHACHA_2], [1],
    [Define to 1 if a second SIMD-optimized ChaCha implementation is
     available.])])
])
