CC=gcc
CFLAGS=-Wall -g -O3 -I.
# -DCHACHARAND_NO_VECS=1

all: src/chacha8.o src/chacha20.o test/test_vectors test/test_stateful

src/chacha8.o: src/chacha.c src/chacha_merged.c src/chacha_krovetz.c src/chacha_merged_ecrypt.h
	$(CC) $(CFLAGS) -DCHACHA_RNDS=8 -c src/chacha.c -o src/chacha8.o

src/chacha20.o: src/chacha.c src/chacha_merged.c src/chacha_krovetz.c src/chacha_merged_ecrypt.h
	$(CC) $(CFLAGS) -DCHACHA_RNDS=20 -c src/chacha.c -o src/chacha20.o

test/test_vectors: test/test_vectors.c src/chacha8.o src/chacha20.o
	$(CC) $(CFLAGS) -Isrc test/test_vectors.c src/chacha8.o src/chacha20.o -o test/test_vectors

test/test_stateful: test/test_stateful.c src/chacha8.o src/chacha20.o
	$(CC) $(CFLAGS) -Isrc test/test_stateful.c src/chacha8.o src/chacha20.o -o test/test_stateful

test/test_vectors.expected: test/make_test_vectors.py
	./test/make_test_vectors.py > test/test_vectors.expected

test/test_vectors.actual: test/test_vectors
	./test/test_vectors > test/test_vectors.actual

check: test/test_vectors.expected test/test_vectors.actual test/test_stateful
	./test/test_stateful
	cmp test/test_vectors.expected test/test_vectors.actual && echo OK

clean:
	rm -f src/*.o test/*.o test/test_vectors
