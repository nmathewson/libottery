
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
  printf("%s: ", label);
  while (n--) {
    printf("%02x", *bytes++);
    if (0 == (++m % 32) && n)
      printf("\n      ");
  }
  puts("");
}

void
experiment(const u8 *key, const u8 *nonce, int skip)
{
  u8 *stream = malloc(skip+256);

  puts("================================================================");
  dumphex("   key", key, 32);
  dumphex(" nonce", nonce, 8);
  printf("@%5d", skip);
  crypto_stream_20(stream, skip+256, key, nonce);
  dumphex("", stream+skip, 256);
  free(stream);
}

#define X(key,nonce,skip) experiment((const u8*)(key),(const u8*)(nonce),(skip))

int
main(int argc, char **v)
{
  X("helloworld!helloworld!helloworld",
    "!hellowo", 0);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 0);
  X("helloworld!helloworld!helloworld",
    "!hellowo", 8192);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 8192);

  return 0;
}
