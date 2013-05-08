#include "chacha.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#ifndef CHACHARAND_NO_LOCKS
#ifdef __APPLE__
#define CHACHARAND_OSATOMIC
#include <libkern/OSAtomic.h>
#else
#define CHACHARAND_PTHREADS
#include <pthread.h>
#endif
#endif

#define u64 uint64_t
#define u32 uint32_t
#define u8 uint8_t

#ifndef CHACHA_RNDS
#define CHACHA_RNDS 20    /* 8 (high speed), 20 (conservative), 12 (middle) */
#endif

#if ! defined(CHACHARAND_NO_VECS)      \
       && (defined(__ARM_NEON__) ||    \
           defined(__ALTIVEC__)  ||    \
           defined(__SSE2__))
#include "src/chacha_krovetz.c"
#else
#include "src/chacha_merged.c"
#endif

#if CHACHA_RNDS == 8
/*HACK*/
void
chacha_state_setup(struct chacha_state *state,
                   const uint8_t *key,
                   const uint8_t *nonce,
                   uint64_t counter)
{
  memcpy(state->key, key, 32);
  memcpy(state->nonce, nonce, 8);
  state->block_counter = counter;
}

int
chacharand_os_bytes(uint8_t *out, size_t outlen)
{
  int fd;
  fd = open("/dev/urandom", O_RDONLY|O_CLOEXEC);
  if (fd < 0)
    return -1;
  if (read(fd, out, outlen) != outlen)
    return -1;
  close(fd);
  return 0;
}
#endif

#ifdef BPI
#define BUFFER_SIZE (BPI*64)
#else
#define BUFFER_SIZE (64)
#endif

#if BUFFER_SIZE > 256
#error "impossible buffer size"
#endif

struct chacharand_state {
  struct chacha_state chst;
  uint8_t buffer[BUFFER_SIZE];
  uint8_t pos;
  uint8_t initialized;
  pid_t pid;
#if defined(CHACHARAND_OSATOMIC)
  OSSpinLock mutex;
#elif defined(CHACHARAND_PTHREADS)
  pthread_mutex_t mutex;
#endif
};

static void
chacharand_memclear(void *mem, size_t len)
{
  /* XXXX Test on many many platforms. */
  volatile uint8_t *cp = mem;
  while (len--)
    *cp++ = 0;
}

#if CHACHA_RNDS == 8
#define chacharand_init chacharand8_init
#define chacharand_add_seed chacharand8_add_seed
#define chacharand_flush chacharand8_flush
#define chacharand_stir chacharand8_stir
#define chacharand_bytes chacharand8_bytes
#define chacharand_stream crypto_stream_8
#define chacharand_wipe chacharand8_wipe
#define chacharand_unsigned chacharand8_unsigned
#define chacharand_uint64 chacharand8_uint64
#define chacharand_range chacharand8_range
#define chacharand_range64 chacharand8_range64
#else
#define chacharand_init chacharand20_init
#define chacharand_add_seed chacharand20_add_seed
#define chacharand_flush chacharand20_flush
#define chacharand_stir chacharand20_stir
#define chacharand_bytes chacharand20_bytes
#define chacharand_stream crypto_stream_20
#define chacharand_wipe chacharand20_wipe
#define chacharand_unsigned chacharand20_unsigned
#define chacharand_uint64 chacharand20_uint64
#define chacharand_range chacharand20_range
#define chacharand_range64 chacharand20_range64
#endif

#if defined(CHACHARAND_PTHREADS)
#define LOCK(st) do {                             \
    pthread_mutex_lock(&(st)->mutex);           \
  } while (0)
#define UNLOCK(st) do {                             \
    pthread_mutex_unlock(&(st)->mutex);             \
  } while (0)
#elif defined(CHACHARAND_OSATOMIC)
#define LOCK(st) do {                           \
    OSSpinLockLock(&(st)->mutex);               \
  } while(0)
#define UNLOCK(st) do {                         \
    OSSpinLockUnlock(&(st)->mutex);            \
  } while(0)
#elif defined(CHACHARAND_NO_LOCKS)
#define LOCK(st) ((void)0)
#define UNLOCK(st) ((void)0)
#else
#error How do I lock?
#endif

int
chacharand_init(struct chacharand_state *st)
{
  uint8_t inp[40];
  memset(st, 0, sizeof(*st));
#ifdef CHACHARAND_PTHREADS
  if (pthread_mutex_init(&st->mutex, NULL))
    return -1;
#endif
  if (chacharand_os_bytes(inp, sizeof(inp)) < 0)
    return -1;
  chacha_state_setup(&st->chst, inp, inp+32, 0);
  chacharand_memclear(inp, sizeof(inp));
  chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
  st->pos=0;
  st->initialized = 1;
  st->pid = getpid();
  return 0;
}

void
chacharand_add_seed(struct chacharand_state *st, uint8_t *seed, size_t n)
{
  uint8_t inp[BUFFER_SIZE];
  LOCK(st);
  while (n) {
    int i;
    size_t m = n % 32;
    for (i = 0; i < m; ++i) {
      st->chst.key[i] ^= seed[i];
      chacharand_stream(inp, BUFFER_SIZE, &st->chst);
      chacha_state_setup(&st->chst, inp, inp+32, 0);
    }
    n -= m;
  }
  chacharand_memclear(inp, sizeof(inp));
  st->pos = 0;
  UNLOCK(st);
}

void
chacharand_wipe(struct chacharand_state *st)
{
#ifdef CHACHARAND_PTHREADS
  pthread_mutex_destroy(&st->mutex);
#endif
  chacharand_memclear(st, sizeof(struct chacharand_state));
}

void
chacharand_flush(struct chacharand_state *st)
{
  LOCK(st);
  chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
  st->pos = 0;
  UNLOCK(st);
}

static void
chacharand_stir_nolock(struct chacharand_state *st)
{
  uint8_t inp[BUFFER_SIZE];
  chacharand_stream(inp, BUFFER_SIZE, &st->chst);
  chacha_state_setup(&st->chst, inp, inp+32, 0);
  chacharand_memclear(inp, sizeof(inp));
  chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
  st->pos=0;
}

void
chacharand_stir(struct chacharand_state *st)
{
  LOCK(st);
  chacharand_stir(st);
  UNLOCK(st);
}

void
chacharand_bytes(struct chacharand_state *st, void *out,
                   size_t n)
{
#ifndef CHACHARAND_NO_INIT_CHECK
  if (!st->initialized)
    abort();
#endif

  LOCK(st);
#ifndef CHACHARAND_NO_PID_CHECK
  if (st->pid != getpid()) {
    if (chacharand_init(st) < 0)
      abort();
  }
#endif

  if (n >= BUFFER_SIZE) {
    size_t remain = n % BUFFER_SIZE;
    chacharand_stream(out, n - remain, &st->chst);
    out += (n - remain);
    n = remain;
  }

  if (n + st->pos < BUFFER_SIZE) {
    memcpy(out, st->buffer+st->pos, n);
    st->pos += n;
  } else {
    memcpy(out, st->buffer+st->pos, BUFFER_SIZE-st->pos);
    n -= (BUFFER_SIZE-st->pos);
    out += (BUFFER_SIZE-st->pos);
    chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
    memcpy(out, st->buffer, n);
    st->pos = n;
  }

  if (st->chst.block_counter > (1<<20))
    chacharand_stir_nolock(st);
  UNLOCK(st);
}

static inline void
chacharand_bytes_small(struct chacharand_state *st, void *out,
      size_t n)
{
#ifndef CHACHARAND_NO_INIT_CHECK
  if (!st->initialized)
    abort();
#endif

  LOCK(st);
#ifndef CHACHARAND_NO_PID_CHECK
  if (st->pid != getpid()) {
    if (chacharand_init(st) < 0)
      abort();
  }
#endif

  if (n + st->pos < BUFFER_SIZE) {
    memcpy(out, st->buffer+st->pos, n);
    st->pos += n;
  } else if (n + st->pos == BUFFER_SIZE) {
    memcpy(out, st->buffer+st->pos, n);
    chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
    st->pos = 0;
    if (st->chst.block_counter > (1<<20))
      chacharand_stir_nolock(st);
  } else {
    chacharand_stream(st->buffer, BUFFER_SIZE, &st->chst);
    memcpy(out, st->buffer, n);
    st->pos = n;
    if (st->chst.block_counter > (1<<20))
      chacharand_stir_nolock(st);
  }

  UNLOCK(st);
}

unsigned
chacharand_unsigned(struct chacharand_state *st)
{
  unsigned u;
  chacharand_bytes_small(st, &u, sizeof(u));
  return u;
}

uint64_t
chacharand_uint64(struct chacharand_state *st)
{
  uint64_t u;
  chacharand_bytes_small(st, &u, sizeof(u));
  return u;
}

unsigned
chacharand_range(struct chacharand_state *st, unsigned upper)
{
  unsigned divisor = UINT_MAX / upper;
  unsigned n;
  do {
    n = (chacharand_unsigned(st) / divisor);
  } while (n > upper);

  return n;
}

uint64_t
chacharand_range64(struct chacharand_state *st, uint64_t upper)
{
  unsigned divisor = UINT64_MAX / upper;
  unsigned n;
  do {
    n = (chacharand_uint64(st) / divisor);
  } while (n > upper);

  return n;
}
