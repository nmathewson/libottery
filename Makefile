CC=gcc
CFLAGS=-Wall -W -Wextra -g -O3 -I. -pthread
# -pedantic --std=c99
# -mfpu=neon
# -pthread
# -DOTTERY_NO_VECS
# -DOTTERY_NO_PID_CHECK
# -DOTTERY_NO_INIT_CHECK
# -DNO_ARC4RANDOM
# -DOTTERY_NO_LOCKS

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

test/dump_bytes: test/dump_bytes.o libottery.a
	$(CC) $(CFLAGS) -Isrc test/dump_bytes.o libottery.a -o test/dump_bytes

check: $(TESTS) test/test_vectors.actual
	./test/test_stateful
	cmp test/test_vectors.expected test/test_vectors.actual && echo OK

clean:
	rm -f src/*.o test/*.o $(TESTS) libottery.a


src/chacha12.o: src/chacha12.c src/ottery-internal.h src/chacha_krovetz.c src/chacha_merged.c
src/chacha20.o: src/chacha20.c src/ottery-internal.h src/chacha_krovetz.c src/chacha_merged.c
src/chacha8.o: src/chacha8.c src/ottery-internal.h src/chacha_krovetz.c src/chacha_merged.c
src/ottery.o: src/ottery.c src/ottery-internal.h src/ottery.h

src/bench_rng.o: test/bench_rng.c src/ottery.h
src/dump_bytes.o: test/dump_bytes.c src/ottery.h
src/test_vectors.o: test/test_vectors.c src/ottery-internal.h src/ottery.h
