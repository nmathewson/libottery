CC=gcc
CFLAGS=-Wall -W -Wextra -g -O3 -pthread
# -pedantic --std=c99
# -mfpu=neon
# -pthread

### define if we're an X86 or X86_64 chip.
X86=1

TESTS =  test/test_vectors test/bench_rng test/dump_bytes test/test_memclear \
	test/test_shallow test/test_deep test/test_spec

all: $(TESTS) libottery.a

##ifdef X86
ARCH_OBJS = src/chacha_krovetz_sse3.o src/chacha_krovetz_sse2.o
CFLAGS_EXTRA = -DOTTERY_HAVE_SSE3_IMPL
##else
##ARCH_OBJS = src/chacha_krovetz.o
##CFLAGS_EXTRA =
##endif

CFLAGS_ALL = $(CFLAGS) $(CFLAGS_EXTRA)
OTTERY_BASE_OBJS = src/chacha_merged.o src/ottery.o \
	src/ottery_osrng.o src/ottery_global.o src/ottery_cpuinfo.o
OTTERY_OBJS = $(OTTERY_BASE_OBJS) $(ARCH_OBJS)
TEST_OBJS = test/test_vectors.o test/bench_rng.o \
	test/dump_bytes.o test/streams.o test/test_memclear.o \
	test/tinytest.o test/test_shallow.o test/test_deep.o \
	test/test_spec.o

UNCRUSTIFY_FILES = src/chacha_merged.c src/chacha_krovetz.c \
	src/ottery.c src/ottery_osrng.c src/ottery_cpuinfo.c \
	src/ottery.h src/ottery_st.h \
	src/ottery-config.h src/ottery-internal.h \
	test/bench_rng.c test/dump_bytes.c test/streams.c test/test_deep.c \
	test/test_spec.c \
	test/test_memclear.c test/test_shallow.c test/test_vectors.c \
	test/streams.h test/st_wrappers.h

libottery.a: $(OTTERY_OBJS)
	ar rcs libottery.a $(OTTERY_OBJS) && ranlib libottery.a

$(OTTERY_BASE_OBJS): %.o: %.c
	$(CC) $(CFLAGS_ALL) -c $< -o $@

$(TEST_OBJS): %.o: %.c
	$(CC) $(CFLAGS_ALL) -Isrc -c $< -o $@

##ifdef X86
src/chacha_krovetz_sse3.o: src/chacha_krovetz.c src/ottery-internal.h \
  src/ottery-config.h
	$(CC) $(CFLAGS_ALL) -msse3 -c $< -o $@
src/chacha_krovetz_sse2.o: src/chacha_krovetz.c src/ottery-internal.h \
  src/ottery-config.h
	$(CC) $(CFLAGS_ALL) -mno-sse3 -c $< -o $@
##else
##src/chacha_krovetz.o: src/chacha_krovetz.c src/ottery-internal.h \
##    src/ottery-config.h
##	$(CC) $(CFLAGS_ALL) -c $< -o $@
##endif


test/test_vectors: test/test_vectors.o test/streams.o libottery.a
	$(CC) $(CFLAGS_ALL) -Isrc test/test_vectors.o test/streams.o libottery.a -o test/test_vectors

test/bench_rng: test/bench_rng.o libottery.a
	$(CC) $(CFLAGS_ALL) -Isrc test/bench_rng.o libottery.a -lcrypto -o test/bench_rng

test/test_vectors.expected: test/make_test_vectors.py
	./test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	./test/test_vectors > test/test_vectors.actual

test/test_vectors.actual-midrange: test/test_vectors
	./test/test_vectors midrange > test/test_vectors.actual-midrange

test/test_vectors.actual-nosimd: test/test_vectors
	./test/test_vectors no-simd > test/test_vectors.actual-nosimd

test/dump_bytes: test/dump_bytes.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/dump_bytes.o libottery.a -o test/dump_bytes

test/test_memclear: test/test_memclear.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_memclear.o libottery.a -o test/test_memclear

test/test_shallow: test/test_shallow.o test/tinytest.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_shallow.o test/tinytest.o libottery.a -o test/test_shallow

test/test_deep: test/test_deep.o test/tinytest.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_deep.o test/tinytest.o libottery.a -o test/test_deep

test/test_spec: test/test_spec.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_spec.o libottery.a -o test/test_spec

test/hs/test_ottery: test/hs/Ottery.hs test/hs/ChaCha.hs test/hs/test_ottery.hs
	(cd test/hs && ghc test_ottery.hs)

check: $(TESTS) test/test_vectors.actual test/test_vectors.actual-nosimd \
	test/test_vectors.actual-midrange \
	test/test_shallow

	@cmp test/test_vectors.expected test/test_vectors.actual && echo OKAY || BAD
	@cmp test/test_vectors.expected test/test_vectors.actual-midrange && echo OKAY || BAD
	@cmp test/test_vectors.expected test/test_vectors.actual-nosimd && echo OKAY || echo BAD
	@./test/test_memclear
	@./test/test_shallow --quiet && echo "OKAY"
	@./test/test_deep --quiet && echo "OKAY"

check-spec: test/hs/test_ottery.output test/test_spec.output
	@cmp test/hs/test_ottery.output test/test_spec.output && echo OKAY || echo BAD

test/hs/test_ottery.output: test/hs/test_ottery test/test_spec
	./test/hs/test_ottery `./test/test_spec --blocks-per-call` > test/hs/test_ottery.output

test/test_spec.output: test/test_spec test/test_spec_seed
	./test/test_spec test/test_spec_seed > test/test_spec.output

clean:
	rm -f src/*.o test/*.o $(TESTS) libottery.a

doxygen:
	doxygen etc/doxygen.conf

uncrustify:
	uncrustify -c etc/uncrustify.cfg --replace -l C $(UNCRUSTIFY_FILES)

src/chacha_krovetz.o: src/chacha_krovetz.c src/ottery-internal.h \
  src/ottery-config.h
src/chacha_merged.o: src/chacha_merged.c src/ottery-internal.h \
  src/ottery-config.h src/chacha_merged_ecrypt.h
src/ottery.o: src/ottery.c src/ottery-internal.h src/ottery-config.h \
  src/ottery.h src/ottery_common.h src/ottery_st.h src/ottery_nolock.h
src/ottery_cpuinfo.o: src/ottery_cpuinfo.c src/ottery-internal.h \
  src/ottery-config.h
src/ottery_global.o: src/ottery_global.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h src/ottery_common.h src/ottery_st.h
src/ottery_osrng.o: src/ottery_osrng.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h src/ottery_common.h

test/bench_rng.o: test/bench_rng.c src/ottery.h src/ottery_common.h \
  src/ottery_st.h src/ottery_nolock.h
test/dump_bytes.o: test/dump_bytes.c src/ottery.h src/ottery_common.h \
  src/ottery_st.h src/ottery_nolock.h test/st_wrappers.h
test/streams.o: test/streams.c test/streams.h src/ottery-internal.h \
  src/ottery-config.h
test/test_deep.o: test/test_deep.c src/ottery.h src/ottery_common.h \
  src/ottery_st.h src/ottery_nolock.h src/ottery-internal.h \
  src/ottery-config.h test/tinytest.h test/tinytest_macros.h \
  test/st_wrappers.h
test/test_memclear.o: test/test_memclear.c
test/test_shallow.o: test/test_shallow.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h src/ottery_common.h src/ottery_st.h \
  src/ottery_nolock.h test/tinytest.h test/tinytest_macros.h \
  test/st_wrappers.h
test/test_spec.o: test/test_spec.c src/ottery_st.h src/ottery_common.h \
  src/ottery-internal.h src/ottery-config.h
test/test_vectors.o: test/test_vectors.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h src/ottery_common.h test/streams.h
test/tinytest.o: test/tinytest.c test/tinytest.h test/tinytest_macros.h
