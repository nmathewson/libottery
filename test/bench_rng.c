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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

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

#include "ottery-internal.h"
#include "ottery.h"
#include "ottery_st.h"
#include "ottery_nolock.h"

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

#define N2 100000

#define TIME_BUF(buf_sz, rng_fn) do {                             \
    struct timeval start, end;                                    \
    unsigned char buf[buf_sz];                                    \
    int i;                                                        \
    gettimeofday(&start, NULL);                                   \
    for (i = 0; i < N2; ++i) {                                    \
      rng_fn;                                                     \
    }                                                             \
    gettimeofday(&end, NULL);                                     \
    uint64_t usec = end.tv_sec - start.tv_sec;                    \
    usec *= 1000000;                                              \
    usec += end.tv_usec - start.tv_usec;                          \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N2); \
                                                                  \
} while (0)


struct ottery_state s8;
struct ottery_state s12;
struct ottery_state s20;
struct ottery_state_nolock s8nl;
struct ottery_state_nolock s12nl;
struct ottery_state_nolock s20nl;

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

#if defined(i386) || \
    defined(__i386) || \
    defined(__x86_64) || \
    defined(__M_IX86) || \
    defined(_M_IX86) || \
    defined(__INTEL_COMPILER)
static unsigned
rdrand(void)
{
  unsigned therand;
  unsigned char status;
  __asm volatile(".byte 0x0F, 0xC7, 0xF0 ; setc %1"
               : "=a" (therand), "=qm" (status));
  assert(status);
  return therand;
}
#endif


#define CHACHA_SUITE(fn_suffix, state, ottery_suffix)                                 \
  void                                                                                \
  time_ ## fn_suffix (void)                                                           \
  {                                                                                   \
    TIME_UNSIGNED_RNG((ottery_st_rand_unsigned ## ottery_suffix (state)));            \
  }                                                                                   \
  void                                                                                \
    time_ ## fn_suffix ## _u64(void)                                                  \
  {                                                                                   \
    TIME_UNSIGNED_RNG((ottery_st_rand_uint64 ## ottery_suffix (state)));              \
  }                                                                                   \
  void                                                                                \
    time_ ## fn_suffix ## _onebyte(void)                                              \
  {                                                                                   \
    TIME_BUF(1, ottery_st_rand_bytes ## ottery_suffix(state, buf, sizeof(buf)));      \
  }                                                                                   \
  void                                                                                \
    time_ ## fn_suffix ## _buf16(void)                                                \
  {                                                                                   \
    TIME_BUF(16, (ottery_st_rand_bytes ## ottery_suffix(state, buf, sizeof(buf))));   \
  }                                                                                   \
  void                                                                                \
    time_ ## fn_suffix ## _buf1024(void)                                              \
  {                                                                                   \
    TIME_BUF(1024, (ottery_st_rand_bytes ## ottery_suffix(state, buf, sizeof(buf)))); \
  }

CHACHA_SUITE(chacharand8, &s8, );
CHACHA_SUITE(chacharand12, &s12, );
CHACHA_SUITE(chacharand20, &s20, );
CHACHA_SUITE(chacharand8nl, &s8nl, _nolock );
CHACHA_SUITE(chacharand12nl, &s12nl, _nolock );
CHACHA_SUITE(chacharand20nl, &s20nl, _nolock );

void
time_rdrandom(void)
{
  TIME_UNSIGNED_RNG(rdrand());
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
#ifdef HAVE_ARC4RANDOM
  TIME_UNSIGNED_RNG((arc4random()));
#endif
}

#ifdef _WIN32
#define libc_random() ((rand()<<16)+rand())
#else
#define libc_random() random()
#endif

void
time_libc_random(void)
{
  TIME_UNSIGNED_RNG((libc_random()));
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
  TIME_UNSIGNED_RNG((((uint64_t)libc_random()) << 32) + libc_random());
}


#ifdef HAVE_ARC4RANDOM_BUF
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
#ifdef HAVE_ARC4RANDOM_BUF
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

static inline void
libc_random_buf(void *b, size_t n)
{
  unsigned *cp = b;
  unsigned i;
  for (i = 0; i < n/sizeof(unsigned); ++i) {
    *cp++ = libc_random();
  }
}

static inline void
rdrandom_buf(void *b, size_t n)
{
  unsigned *cp = b;
  unsigned i;
  for (i = 0; i < n/sizeof(unsigned); ++i) {
    *cp++ = rdrand();
  }
}

void
time_arc4random_onebyte(void)
{
#if HAVE_ARC4RANDOM_BUF
  TIME_BUF(1, arc4random_buf(buf, sizeof(buf)));
#endif
}

void
time_libc_onebyte(void)
{
#ifdef _WIN32
  TIME_UNSIGNED_RNG((rand() & 0xff));
#else
  TIME_UNSIGNED_RNG((libc_random() & 0xff));
#endif
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
#ifdef HAVE_ARC4RANDOM_BUF
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
time_rdrandom_buf16(void)
{
  TIME_BUF(16, rdrandom_buf(buf, sizeof(buf)));
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
#ifdef HAVE_ARC4RANDOM_BUF
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

void
time_rdrandom_buf1024(void)
{
  TIME_BUF(1024, rdrandom_buf(buf, sizeof(buf)));
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
  ottery_st_init_nolock(&s8nl, &cfg_chacha8);
  ottery_st_init_nolock(&s12nl, &cfg_chacha12);
  ottery_st_init_nolock(&s20nl, &cfg_chacha20);

  time_chacharand8();
  time_chacharand8_u64();
  time_chacharand8_onebyte();
  time_chacharand8_buf16();
  time_chacharand8_buf1024();

  time_chacharand12();
  time_chacharand12_u64();
  time_chacharand12_onebyte();
  time_chacharand12_buf16();
  time_chacharand12_buf1024();

  time_chacharand20();
  time_chacharand20_u64();
  time_chacharand20_onebyte();
  time_chacharand20_buf16();
  time_chacharand20_buf1024();

  time_chacharand8nl();
  time_chacharand8nl_u64();
  time_chacharand8nl_onebyte();
  time_chacharand8nl_buf16();
  time_chacharand8nl_buf1024();

  time_chacharand12nl();
  time_chacharand12nl_u64();
  time_chacharand12nl_onebyte();
  time_chacharand12nl_buf16();
  time_chacharand12nl_buf1024();

  time_chacharand20nl();
  time_chacharand20nl_u64();
  time_chacharand20nl_onebyte();
  time_chacharand20nl_buf16();
  time_chacharand20nl_buf1024();

  time_arc4random();
  time_arc4random_u64();
  time_arc4random_onebyte();
  time_arc4random_buf16();
  time_arc4random_buf1024();

  if (ottery_get_cpu_capabilities_() & OTTERY_CPUCAP_RAND) {
    time_rdrandom();
    time_rdrandom_buf16();
    time_rdrandom_buf1024();
  }

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

