#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef NO_OPENSSL
#define OPENSSL
#include <openssl/rand.h>
#endif

#include "ottery.h"

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

struct ottery_state s8;
struct ottery_state s20;

void
time_chacharand8(void)
{
  TIME_UNSIGNED_RNG( (ottery_st_rand_unsigned(&s8)) );
}

void
time_chacharand20(void)
{
  TIME_UNSIGNED_RNG( (ottery_st_rand_unsigned(&s20)) );
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
  TIME_BUF(16, (ottery_st_rand_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand20_buf16(void)
{
  TIME_BUF(16, (ottery_st_rand_bytes(&s20, buf, sizeof(buf))));
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
  TIME_BUF(1024, (ottery_st_rand_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand20_buf1024(void)
{
  TIME_BUF(1024, (ottery_st_rand_bytes(&s20, buf, sizeof(buf))));
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

int
main(int argc, char **argv)
{
#ifdef OPENSSL
  RAND_poll();
#endif
  struct ottery_config cfg_chacha8;
  struct ottery_config cfg_chacha20;
  ottery_config_init(&cfg_chacha8);
  ottery_config_force_implementation(&cfg_chacha8, OTTERY_CHACHA8);
  ottery_config_init(&cfg_chacha20);
  ottery_config_force_implementation(&cfg_chacha20, OTTERY_CHACHA20);

  ottery_st_init(&s8, &cfg_chacha8);
  ottery_st_init(&s20, &cfg_chacha20);

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

