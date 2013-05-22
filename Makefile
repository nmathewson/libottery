CC=gcc
CFLAGS=-Wall -W -Wextra -g -O3 -pthread
# -pedantic --std=c99
# -mfpu=neon
# -pthread

TESTS =  test/test_vectors test/bench_rng test/dump_bytes test/test_memclear \
	test/test_shallow test/test_deep

all: $(TESTS) libottery.a

OTTERY_OBJS = src/chacha8.o src/chacha12.o src/chacha20.o src/ottery.o \
	src/ottery_osrng.o
TEST_OBJS = test/test_vectors.o test/bench_rng.o \
	test/dump_bytes.o test/streams.o test/test_memclear.o \
	test/tinytest.o test/test_shallow.o test/test_deep.o

UNCRUSTIFY_FILES = src/chacha8.c src/chacha12.c src/chacha20.c \
	src/ottery.c src/ottery_osrng.c src/ottery.h src/ottery_st.h \
	src/ottery-config.h src/ottery-internal.h \
	test/bench_rng.c test/dump_bytes.c test/streams.c test/test_deep.c \
	test/test_memclear.c test/test_shallow.c test/test_vectors.c \
	test/streams.h test/st_wrappers.h

libottery.a: $(OTTERY_OBJS)
	ar rcs libottery.a $(OTTERY_OBJS) && ranlib libottery.a

$(OTTERY_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test/test_vectors: test/test_vectors.o test/streams.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_vectors.o test/streams.o libottery.a -o test/test_vectors

test/bench_rng: test/bench_rng.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/bench_rng.o libottery.a -lcrypto -o test/bench_rng

test/test_vectors.expected: test/make_test_vectors.py
	./test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	./test/test_vectors > test/test_vectors.actual

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

check: $(TESTS) test/test_vectors.actual test/test_vectors.actual-nosimd \
	test/test_shallow

	@cmp test/test_vectors.expected test/test_vectors.actual && echo OKAY || BAD
	@cmp test/test_vectors.expected test/test_vectors.actual-nosimd && echo OKAY || echo BAD
	@./test/test_memclear
	@./test/test_shallow --quiet && echo "OKAY"
	@./test/test_deep --quiet && echo "OKAY"

clean:
	rm -f src/*.o test/*.o $(TESTS) libottery.a

doxygen:
	doxygen etc/doxygen.conf

uncrustify:
	uncrustify -c etc/uncrustify.cfg --replace -l C $(UNCRUSTIFY_FILES)

src/chacha12.o: src/chacha12.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/chacha20.o: src/chacha20.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/chacha8.o: src/chacha8.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/ottery.o: src/ottery.c src/ottery-internal.h src/ottery-config.h \
  src/ottery.h src/ottery_st.h
src/ottery_osrng.o: src/ottery_osrng.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h

test/bench_rng.o: test/bench_rng.c src/ottery.h src/ottery_st.h
dump_bytes.o: test/dump_bytes.c src/ottery.h src/ottery_st.h \
  test/st_wrappers.h
test/streams.o: test/streams.c test/streams.h src/ottery-internal.h \
  src/ottery-config.h
test/test_memclear.o: test/test_memclear.c
test_shallow.o: test/test_shallow.c src/ottery.h src/ottery_st.h \
  src/ottery-internal.h src/ottery-config.h test/tinytest.h \
  test/tinytest_macros.h test/st_wrappers.h
test/test_vectors.o: test/test_vectors.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h test/streams.h
test/tinytest.o: test/tinytest.c test/tinytest.h test/tinytest_macros.h

