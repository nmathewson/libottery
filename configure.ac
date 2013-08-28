
AC_INIT(ottery, 0.0.0-dev)
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR(src/ottery.c)

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
dnl AM_SILENT_RULES req. automake 1.11.  [no] defaults V=1
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_DEFINE(NUMERIC_VERSION, 0x00000000, [Numeric representation of the version])

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

AM_CONDITIONAL([X86], [true])

AC_CONFIG_FILES( [libottery.pc] )
AC_OUTPUT(Makefile)

