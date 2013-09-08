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
