CC=gcc
CFLAGS=-Wall -g -O3 -I. -pthread
# -mfpu=neon
#  -pthread
# -DOTTERY_NO_VECS

TESTS =  test/test_vectors test/test_stateful test/bench_rng

all: $(TESTS) libottery.a

OTTERY_OBJS = src/chacha8.o src/chacha12.o src/chacha20.o src/ottery.o

libottery.a: $(OTTERY_OBJS)
	ar rcs libottery.a $(OTTERY_OBJS) && ranlib libottery.a

$(OTTERY_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test/test_vectors: test/test_vectors.c libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_vectors.c libottery.a -o test/test_vectors

test/test_stateful: test/test_stateful.c libottery.a
	$(CC) $(CFLAGS) -Isrc test/test_stateful.c libottery.a -o test/test_stateful

test/bench_rng: test/bench_rng.c libottery.a
	$(CC) $(CFLAGS) -Isrc test/bench_rng.c libottery.a -lcrypto -o test/bench_rng

test/test_vectors.expected: test/make_test_vectors.py
	./test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	./test/test_vectors > test/test_vectors.actual

test/dump_bytes: test/dump_bytes.c libottery.a
	$(CC) $(CFLAGS) -Isrc test/dump_bytes.c libottery.a -o test/dump_bytes

check: $(TESTS)
	./test/test_stateful
	cmp test/test_vectors.expected test/test_vectors.actual && echo OK

clean:
	rm -f src/*.o test/*.o $(TESTS) libottery.a
