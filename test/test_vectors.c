#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int crypto_stream_8(unsigned char *out,
                  unsigned long long inlen,
                  const unsigned char *n,
                  const unsigned char *k);

int crypto_stream_20(unsigned char *out,
                  unsigned long long inlen,
                  const unsigned char *n,
                  const unsigned char *k);

typedef unsigned char u8;

void
dumphex(const char *label, const u8 *bytes, int n)
{
  int m=0;
  if (label) printf("%s: ", label);
  while (n--) {
    printf("%02x", *bytes++);
    if (0 == (++m % 32) && n)
      printf("\n");
  }
  puts("");
}

void
experiment(const u8 *key, const u8 *nonce, int skip, int rounds)
{
  u8 *stream = malloc(skip+256);

  puts("================================================================");
  dumphex("   key", key, 32);
  dumphex(" nonce", nonce, 8);
  printf ("offset: %d, rounds: %d\n", skip, rounds);
  if (rounds == 8)
    crypto_stream_8(stream, skip+256, nonce, key);
  else if (rounds == 20)
    crypto_stream_20(stream, skip+256, nonce, key);
  else
    memset(stream, 255, skip+256);
  dumphex(NULL, stream+skip, 256);
  free(stream);
}

#define X(key,nonce,skip) do {                                  \
    experiment((const u8*)(key),(const u8*)(nonce),(skip), 8);  \
    experiment((const u8*)(key),(const u8*)(nonce),(skip), 20); \
  } while (0)

int
main(int argc, char **v)
{
  X("helloworld!helloworld!helloworld", "!hellowo", 0);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 0);
  X("helloworld!helloworld!helloworld", "!hellowo", 8192);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 8192);
  X("Zombie ipsum reversus ab viral i", "nferno, ", 128);
  X("nam rick grimes malum cerebro. D", "e carne ", 512);
  X("lumbering animata corpora quaeri", "tis. Sum", 640);
  X("mus brains sit, morbo vel malefi", "cia? De ", 704);

  return 0;
}
