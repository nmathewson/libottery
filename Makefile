CC=gcc
CFLAGS=-Wall -g -O3 -I.
#CFLAGS=$(CFLAGS) -DCHACHARAND_NO_VECS=1

all: src/chacha.o test/test_vectors

src/chacha.o: src/chacha.c src/chacha_merged.c src/chacha_krovetz.c src/chacha_merged_ecrypt.h
	$(CC) $(CFLAGS) -c src/chacha.c -o src/chacha.o

test/test_vectors: test/test_vectors.c src/chacha.o
	$(CC) $(CFLAGS) test/test_vectors.c src/chacha.o -o test/test_vectors

clean:
	rm -f src/chacha.o
