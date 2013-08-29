AUTOMAKE_OPTIONS = foreign 1.9 subdir-objects

ACLOCAL_AMFLAGS = -I m4

# To increment a VERSION_INFO (current:revision:age):
#    If the ABI didn't change:
#        Return (current:revision+1:age)
#    If the ABI changed, but it's backward-compatible:
#        Return (current+1:0:age+1)
#    If the ABI changed and it isn't backward-compatible:
#        Return (current+1:0:0)
#
VERSION_INFO = 0:0:0

TEST_PROGS = test/test_vectors test/bench_rng test/dump_bytes	\
	test/test_memclear test/test_shallow test/test_deep	\
	test/test_spec

TESTS = test/test_memclear test/test_shallow test/test_deep \
	test/test_vector_cmp.sh

check_PROGRAMS = $(TEST_PROGS)
lib_LTLIBRARIES = libottery.la
pkgconfig_DATA  = libottery.pc
include_HEADERS	=				\
	src/ottery.h				\
	src/ottery_common.h			\
	src/ottery_nolock.h			\
	src/ottery_st.h

noinst_HEADERS = 			\
	src/chacha_merged_ecrypt.h 	\
	src/ottery-internal.h 		\
	test/st_wrappers.h 		\
	test/streams.h 			\
	test/tinytest.h 		\
	test/tinytest_macros.h

pkgconfigdir=$(libdir)/pkgconfig

GENERIC_LDFLAGS = -version-info $(VERSION_INFO)

OTTERY_CORE_SRC =				\
	src/chacha_merged.c			\
	src/ottery.c				\
	src/ottery_cpuinfo.c			\
	src/ottery_global.c			\
	src/ottery_osrng.c

if X86
OTTERY_ARCH_SRC =
OTTERY_LIBS_EXTRA = libchacha-sse3.la libchacha-sse2.la
noinst_LTLIBRARIES = $(OTTERY_LIBS_EXTRA)
libchacha_sse3_la_SOURCES = src/chacha_krovetz.c
libchacha_sse2_la_SOURCES = src/chacha_krovetz.c
libchacha_sse3_la_CFLAGS = -msse3 -DOTTERY_BUILDING_SSE3_IMPL
libchacha_sse2_la_CFLAGS = -msse2 -mno-sse3
CFLAGS_EXTRA = -DOTTERY_HAVE_SSE3_IMPL
else
OTTERY_ARCH_SRC = src/chacha_krovetz.c
CFLAGS_EXTRA =
OTTERY_LIBS_EXTRA =
endif

AM_CFLAGS = $(CFLAGS_EXTRA) -I $(top_srcdir)/src

libottery_la_SOURCES = $(OTTERY_CORE_SRC) $(OTTERY_ARCH_SRC)
libottery_la_LDFLAGS = $(GENERIC_LDFLAGS)
libottery_la_LIBADD = $(OTTERY_LIBS_EXTRA)

test_test_vectors_SOURCES = test/test_vectors.c test/streams.c
test_test_vectors_LDADD = libottery.la

test_bench_rng_SOURCES = test/bench_rng.c
test_bench_rng_LDADD = libottery.la -lcrypto

noinst_DATA =					\
	test/test_vectors.expected		\
	test/test_vectors.actual		\
	test/test_vectors.actual-nosimd		\
	test/test_vectors.actual-midrange

noinst_SCRIPTS = test/make_test_vectors.py
EXTRA_DIST = test/make_test_vectors.py test/test_vector_cmp.sh

CLEANFILES = $(noinst_DATA)

test/test_vectors.expected: $(top_srcdir)/test/make_test_vectors.py
	$(AM_V_GEN)$(top_srcdir)/test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	$(AM_V_GEN)./test/test_vectors > test/test_vectors.actual

test/test_vectors.actual-midrange: test/test_vectors
	$(AM_V_GEN)./test/test_vectors midrange > test/test_vectors.actual-midrange

test/test_vectors.actual-nosimd: test/test_vectors
	$(AM_V_GEN)./test/test_vectors no-simd > test/test_vectors.actual-nosimd

test_dump_bytes_SOURCES = test/dump_bytes.c
test_dump_bytes_LDADD = libottery.la

test_test_memclear_SOURCES = test/test_memclear.c
test_test_memclear_LDADD = libottery.la

test_test_shallow_SOURCES = test/test_shallow.c test/tinytest.c
test_test_shallow_LDADD = libottery.la

test_test_deep_SOURCES = test/test_deep.c test/tinytest.c
test_test_deep_LDADD = libottery.la

test_test_spec_SOURCES = test/test_spec.c
test_test_spec_LDADD = libottery.la

test/hs/test_ottery: test/hs/Ottery.hs test/hs/ChaCha.hs test/hs/test_ottery.hs
	$(AM_V_GEN)(cd test/hs && ghc test_ottery.hs)

check-spec: test/hs/test_ottery.output test/test_spec.output
	@cmp test/hs/test_ottery.output test/test_spec.output && echo OKAY || echo BAD

test/hs/test_ottery.output: test/hs/test_ottery test/test_spec
	$(AM_V_GEN)./test/hs/test_ottery `./test/test_spec --blocks-per-call` > test/hs/test_ottery.output

test/test_spec.output: test/test_spec test/test_spec_seed
	$(AM_V_GEN)./test/test_spec test/test_spec_seed > test/test_spec.output

doxygen:
	doxygen etc/doxygen.conf

