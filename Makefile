CC=gcc
CFLAGS=-Wall -g -O3 -I. -DCHACHARAND_NO_VECS=1

all: src/chacha.o

src/chacha.o: src/chacha.c src/chacha_merged.c src/chacha_krovetz.c src/chacha_merged_ecrypt.h
	$(CC) $(CFLAGS) -c src/chacha.c -o src/chacha.o

