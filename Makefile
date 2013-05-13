CC=gcc
CFLAGS=-Wall -W -Wextra -g -O3 -pthread
# -pedantic --std=c99
# -mfpu=neon
# -pthread
# -DNO_ARC4RANDOM

TESTS =  test/test_vectors test/bench_rng test/dump_bytes

all: $(TESTS) libottery.a

OTTERY_OBJS = src/chacha8.o src/chacha12.o src/chacha20.o src/ottery.o
TEST_OBJS = test/test_vectors.o test/bench_rng.o \
	test/dump_bytes.o test/streams.o

libottery.a: $(OTTERY_OBJS)
	ar rcs libottery.a $(OTTERY_OBJS) && ranlib libottery.a

$(OTTERY_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test/test_vectors: test/test_vectors.o test/streams.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_vectors.o test/streams.o libottery.a -o test/test_vectors

test/bench_rng: test/bench_rng.o libottery.a
	$(CC) $(CFLAGS) -Wno-deprecated-declarations -Isrc test/bench_rng.o libottery.a -lcrypto -o test/bench_rng

test/test_vectors.expected: test/make_test_vectors.py
	./test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	./test/test_vectors > test/test_vectors.actual

test/test_vectors.actual-nosimd: test/test_vectors
	./test/test_vectors no-simd > test/test_vectors.actual-nosimd

test/dump_bytes: test/dump_bytes.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/dump_bytes.o libottery.a -o test/dump_bytes

check: $(TESTS) test/test_vectors.actual test/test_vectors.actual-nosimd
	cmp test/test_vectors.expected test/test_vectors.actual && echo OK
	cmp test/test_vectors.expected test/test_vectors.actual-nosimd && echo OK

clean:
	rm -f src/*.o test/*.o $(TESTS) libottery.a


src/chacha12.o: src/chacha12.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/chacha20.o: src/chacha20.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/chacha8.o: src/chacha8.c src/ottery-internal.h src/ottery-config.h \
  src/chacha_krovetz.c src/chacha_merged.c src/chacha_merged_ecrypt.h
src/ottery.o: src/ottery.c src/ottery-internal.h src/ottery-config.h \
  src/ottery.h

test/bench_rng.o: test/bench_rng.c src/ottery.h
test/dump_bytes.o: test/dump_bytes.c src/ottery.h
test/streams.o: test/streams.c test/streams.h src/ottery-internal.h \
  src/ottery-config.h
test/test_shallow.o: test/test_shallow.c src/ottery.h
test/test_vectors.o: test/test_vectors.c src/ottery-internal.h \
  src/ottery-config.h src/ottery.h test/streams.h

