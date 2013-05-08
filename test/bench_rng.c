#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef NO_OPENSSL
#define OPENSSL
#include <openssl/rand.h>
#endif

#include "chacha.h"

struct chacharand_state {
  uint8_t junk[1024];
};

struct chacharand_state s8;
struct chacharand_state s20;

int chacharand8_init(struct chacharand_state *st);
int chacharand20_init(struct chacharand_state *st);
unsigned chacharand8_unsigned(struct chacharand_state *st);
unsigned chacharand20_unsigned(struct chacharand_state *st);
void chacharand8_bytes(struct chacharand_state *st, void *bytes, size_t n);
void chacharand20_bytes(struct chacharand_state *st, void *bytes, size_t n);

#define N 10000000

#define TIME_UNSIGNED_RNG(rng_fn) do {          \
    struct timeval start, end;                  \
    unsigned accumulator = 0;                   \
    int i;                                      \
    gettimeofday(&start, NULL);                 \
    for (i=0; i < N; ++i) {                     \
      accumulator += rng_fn;                    \
    }                                           \
    gettimeofday(&end, NULL);                   \
    uint64_t usec = end.tv_sec - start.tv_sec;  \
    usec *= 1000000;\
    usec += end.tv_usec - start.tv_usec;        \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N); \
  } while (0)

void
time_chacharand8(void)
{
  TIME_UNSIGNED_RNG( (chacharand8_unsigned(&s8)) );
}

void
time_chacharand20(void)
{
  TIME_UNSIGNED_RNG( (chacharand20_unsigned(&s20)) );
}

void
time_arc4random(void)
{
  TIME_UNSIGNED_RNG( (arc4random()) );
}

void
time_libc_random(void)
{
  TIME_UNSIGNED_RNG( (random()) );
}

static inline unsigned
openssl_random(void)
{
  unsigned u;
  RAND_bytes(&u, sizeof(u));
  return u;
}
void
time_openssl_random(void)
{
  TIME_UNSIGNED_RNG( (openssl_random()) );
}

#undef N
#define N 100000

#define TIME_BUF(buf_sz, rng_fn) do {                \
    struct timeval start, end;                  \
    unsigned char buf[buf_sz];                  \
    int i;                                      \
    gettimeofday(&start, NULL);                 \
    for (i=0; i < N; ++i) {                     \
      rng_fn;                                   \
    }                                           \
    gettimeofday(&end, NULL);                   \
    uint64_t usec = end.tv_sec - start.tv_sec;  \
    usec *= 1000000;\
    usec += end.tv_usec - start.tv_usec;        \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N); \
                                                  \
  } while (0)

static inline void
libc_random_buf(void *b, size_t n)
{
  unsigned *cp = b;
  int i;
  for (i=0;i<n/sizeof(unsigned);++i) {
    *cp++ = random();
  }
}

void
time_chacharand8_buf16(void)
{
  TIME_BUF(16, (chacharand8_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand20_buf16(void)
{
  TIME_BUF(16, (chacharand20_bytes(&s8, buf, sizeof(buf))));
}
void
time_arc4random_buf16(void)
{
  TIME_BUF(16, (arc4random_buf(buf, sizeof(buf))));
}
void
time_libcrandom_buf16(void)
{
  TIME_BUF(16, (libc_random_buf(buf, sizeof(buf))));
}
void
time_opensslrandom_buf16(void)
{
  TIME_BUF(16, (RAND_bytes(buf, sizeof(buf))));
}

void
time_chacharand8_buf1024(void)
{
  TIME_BUF(1024, (chacharand8_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand20_buf1024(void)
{
  TIME_BUF(1024, (chacharand20_bytes(&s8, buf, sizeof(buf))));
}
void
time_arc4random_buf1024(void)
{
  TIME_BUF(1024, (arc4random_buf(buf, sizeof(buf))));
}
void
time_libcrandom_buf1024(void)
{
  TIME_BUF(1024, (libc_random_buf(buf, sizeof(buf))));
}
void
time_opensslrandom_buf1024(void)
{
  TIME_BUF(1024, (RAND_bytes(buf, sizeof(buf))));
}



int main(int argc, char **argv)
{
#ifdef OPENSSL
  RAND_poll();
#endif
  chacharand8_init(&s8);
  chacharand20_init(&s20);

  time_chacharand8();
  time_chacharand20();
  time_arc4random();
  time_openssl_random();
  time_libc_random();

  time_chacharand8_buf16();
  time_chacharand20_buf16();
  time_arc4random_buf16();
  time_libcrandom_buf16();
  time_opensslrandom_buf16();

  time_chacharand8_buf1024();
  time_chacharand20_buf1024();
  time_arc4random_buf1024();
  time_libcrandom_buf1024();
  time_opensslrandom_buf1024();

  return 0;
}

