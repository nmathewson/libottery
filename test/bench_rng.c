/* Libottery by Nick Mathewson.

   This software has been dedicated to the public domain under the CC0
   public domain dedication.

   To the extent possible under law, the person who associated CC0 with
   libottery has waived all copyright and related or neighboring rights
   to libottery.

   You should have received a copy of the CC0 legalcode along with this
   work in doc/cc0.txt.  If not, see
      <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#if !(                    \
  defined(__APPLE__) ||   \
  defined(__FreeBSD__) || \
  defined(__NetBSD__))
#define NO_ARC4RANDOM
#endif

#ifdef _WIN32
#define NO_URANDOM
#else
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef NO_OPENSSL
#define OPENSSL
#if defined(__APPLE__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <openssl/rand.h>
#endif

#include "ottery.h"
#include "ottery_st.h"

#define N 10000000

#define TIME_UNSIGNED_RNG(rng_fn) do {                           \
    struct timeval start, end;                                   \
    unsigned accumulator = 0;                                    \
    int i;                                                       \
    gettimeofday(&start, NULL);                                  \
    for (i = 0; i < N; ++i) {                                    \
      accumulator += (unsigned)(rng_fn);                         \
    }                                                            \
    gettimeofday(&end, NULL);                                    \
    uint64_t usec = end.tv_sec - start.tv_sec;                   \
    usec *= 1000000;                                             \
    usec += end.tv_usec - start.tv_usec;                         \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N); \
} while (0)

struct ottery_state s8;
struct ottery_state s12;
struct ottery_state s20;

#ifndef NO_URANDOM
int urandom_fd = -1;

static void
urandom_buf(void *buf, size_t n)
{
  read(urandom_fd, buf, n);
}
static unsigned
urandom_unsigned(void)
{
  unsigned u;
  read(urandom_fd, (void*)&u, sizeof(u));
  return u;
}
static uint64_t
urandom_u64(void)
{
  uint64_t u;
  read(urandom_fd, (void*)&u, sizeof(u));
  return u;
}
#endif

void
time_chacharand8(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_unsigned(&s8)));
}

void
time_chacharand12(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_unsigned(&s12)));
}

void
time_chacharand20(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_unsigned(&s20)));
}

#ifndef NO_URANDOM
void
time_urandom(void)
{
  TIME_UNSIGNED_RNG((urandom_unsigned()));
}
#endif

void
time_arc4random(void)
{
#ifndef NO_ARC4RANDOM
  TIME_UNSIGNED_RNG((arc4random()));
#endif
}

void
time_libc_random(void)
{
  TIME_UNSIGNED_RNG((random()));
}

void
time_chacharand8_u64(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_uint64(&s8)));
}

void
time_chacharand12_u64(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_uint64(&s8)));
}

void
time_chacharand20_u64(void)
{
  TIME_UNSIGNED_RNG((ottery_st_rand_uint64(&s20)));
}
#ifndef NO_URANDOM
void
time_urandom_u64(void)
{
  TIME_UNSIGNED_RNG((urandom_u64()));
}
#endif
void
time_libc_random_u64(void)
{
  TIME_UNSIGNED_RNG((((uint64_t)random()) << 32) + random());
}


#ifndef NO_ARC4RANDOM
static inline uint64_t
arc4random_u64(void)
{
  uint64_t x;
  arc4random_buf((void*)&x, sizeof(x));
  return x;
}
#endif

void
time_arc4random_u64(void)
{
#ifndef NO_ARC4RANDOM
  TIME_UNSIGNED_RNG((arc4random_u64()));
#endif
}

#ifndef NO_OPENSSL
static inline unsigned
openssl_random(void)
{
  unsigned u;
  RAND_bytes((void*)&u, sizeof(u));
  return u;
}
#endif
void
time_openssl_random(void)
{
#ifndef NO_OPENSSL
  TIME_UNSIGNED_RNG((openssl_random()));
#endif
}

#undef N
#define N 100000

#define TIME_BUF(buf_sz, rng_fn) do {                            \
    struct timeval start, end;                                   \
    unsigned char buf[buf_sz];                                   \
    int i;                                                       \
    gettimeofday(&start, NULL);                                  \
    for (i = 0; i < N; ++i) {                                    \
      rng_fn;                                                    \
    }                                                            \
    gettimeofday(&end, NULL);                                    \
    uint64_t usec = end.tv_sec - start.tv_sec;                   \
    usec *= 1000000;                                             \
    usec += end.tv_usec - start.tv_usec;                         \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N); \
                                                                 \
} while (0)

static inline void
libc_random_buf(void *b, size_t n)
{
  unsigned *cp = b;
  unsigned i;
  for (i = 0; i < n/sizeof(unsigned); ++i) {
    *cp++ = random();
  }
}


void
time_chacha8_onebyte(void)
{
  TIME_BUF(1, ottery_st_rand_bytes(&s8, buf, sizeof(buf)));
}
void
time_chacha12_onebyte(void)
{
  TIME_BUF(1, ottery_st_rand_bytes(&s12, buf, sizeof(buf)));
}
void
time_chacha20_onebyte(void)
{
  TIME_BUF(1, ottery_st_rand_bytes(&s20, buf, sizeof(buf)));
}
void
time_arc4random_onebyte(void)
{
#ifndef NO_ARC4RANDOM
  TIME_BUF(1, arc4random_buf(buf, sizeof(buf)));
#endif
}
void
time_libc_onebyte(void)
{
  TIME_UNSIGNED_RNG((random() & 0xff));
}

void
time_chacharand8_buf16(void)
{
  TIME_BUF(16, (ottery_st_rand_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand12_buf16(void)
{
  TIME_BUF(16, (ottery_st_rand_bytes(&s12, buf, sizeof(buf))));
}
void
time_chacharand20_buf16(void)
{
  TIME_BUF(16, (ottery_st_rand_bytes(&s20, buf, sizeof(buf))));
}
#ifndef NO_URANDOM
void
time_urandom_buf16(void)
{
  TIME_BUF(16, (urandom_buf(buf, sizeof(buf))));
}
#endif
void
time_arc4random_buf16(void)
{
#ifndef NO_ARC4RANDOM
  TIME_BUF(16, (arc4random_buf(buf, sizeof(buf))));
#endif
}
void
time_libcrandom_buf16(void)
{
  TIME_BUF(16, (libc_random_buf(buf, sizeof(buf))));
}
void
time_opensslrandom_buf16(void)
{
#ifndef NO_OPENSSL
  TIME_BUF(16, (RAND_bytes(buf, sizeof(buf))));
#endif
}

void
time_chacharand8_buf1024(void)
{
  TIME_BUF(1024, (ottery_st_rand_bytes(&s8, buf, sizeof(buf))));
}
void
time_chacharand12_buf1024(void)
{
  TIME_BUF(1024, (ottery_st_rand_bytes(&s12, buf, sizeof(buf))));
}
void
time_chacharand20_buf1024(void)
{
  TIME_BUF(1024, (ottery_st_rand_bytes(&s20, buf, sizeof(buf))));
}
#ifndef NO_URANDOM
void
time_urandom_buf1024(void)
{
  TIME_BUF(1024, (urandom_buf(buf, sizeof(buf))));
}
#endif
void
time_arc4random_buf1024(void)
{
#ifndef NO_ARC4RANDOM
  TIME_BUF(1024, (arc4random_buf(buf, sizeof(buf))));
#endif
}
void
time_libcrandom_buf1024(void)
{
  TIME_BUF(1024, (libc_random_buf(buf, sizeof(buf))));
}
void
time_opensslrandom_buf1024(void)
{
#ifndef NO_OPENSSL
  TIME_BUF(1024, (RAND_bytes(buf, sizeof(buf))));
#endif
}

int
main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
#ifndef NO_OPENSSL
  RAND_poll();
#endif
  struct ottery_config cfg_chacha8;
  struct ottery_config cfg_chacha12;
  struct ottery_config cfg_chacha20;
  ottery_config_init(&cfg_chacha8);
  ottery_config_force_implementation(&cfg_chacha8, OTTERY_PRF_CHACHA8);
  ottery_config_init(&cfg_chacha12);
  ottery_config_force_implementation(&cfg_chacha12, OTTERY_PRF_CHACHA12);
  ottery_config_init(&cfg_chacha20);
  ottery_config_force_implementation(&cfg_chacha20, OTTERY_PRF_CHACHA20);

  ottery_st_init(&s8, &cfg_chacha8);
  ottery_st_init(&s12, &cfg_chacha12);
  ottery_st_init(&s20, &cfg_chacha20);

  time_chacharand8();
  time_chacharand8_u64();
  time_chacha8_onebyte();
  time_chacharand8_buf16();
  time_chacharand8_buf1024();

  time_chacharand12();
  time_chacharand12_u64();
  time_chacha12_onebyte();
  time_chacharand12_buf16();
  time_chacharand12_buf1024();

  time_chacharand20();
  time_chacharand20_u64();
  time_chacha20_onebyte();
  time_chacharand20_buf16();
  time_chacharand20_buf1024();

  time_arc4random();
  time_arc4random_u64();
  time_arc4random_onebyte();
  time_arc4random_buf16();
  time_arc4random_buf1024();

#ifndef NO_URANDOM
  urandom_fd = open("/dev/urandom", O_RDONLY);
  time_urandom();
  time_urandom_u64();
  time_urandom_buf16();
  time_urandom_buf1024();
#endif

  time_libc_random();
  time_libc_random_u64();
  time_libc_onebyte();
  time_libcrandom_buf16();
  time_libcrandom_buf1024();

  time_openssl_random();
  time_opensslrandom_buf16();
  time_opensslrandom_buf1024();

  return 0;
}

