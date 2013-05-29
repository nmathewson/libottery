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
#define OTTERY_INTERNAL
#include "ottery-internal.h"
#include "ottery.h"
#include "ottery_st.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

/* I've added a few assertions to sanity-check for debugging, but they should
 * never ever ever trigger.  It's fine to build this code with NDEBUG. */
#include <assert.h>

#ifdef _WIN32
/* On Windows, there is no fork(), so we don't need to worry about forking. */
#define OTTERY_NO_PID_CHECK
#endif

/**
 * Evaluate the condition 'x', while hinting to the compiler that it is
 * likely to be false.
 */
#define UNLIKELY(x) __builtin_expect((x), 0)

/** Magic number for deciding whether an ottery_state is initialized. */
#define MAGIC_BASIS 0x11b07734

/** Macro: yield the correct magic number for an ottery_state, based on
 * its position in RAM. */
#define MAGIC(ptr) (((uint32_t)(uintptr_t)(ptr)) ^ MAGIC_BASIS)

static inline int ottery_st_rand_lock_and_check(struct ottery_state *st)
__attribute__((always_inline));
static void ottery_st_stir_nolock(struct ottery_state *st);
#ifndef OTTERY_NO_WIPE_STACK
static void ottery_wipe_stack_(void) __attribute__((noinline));
#endif

size_t
ottery_get_sizeof_config(void)
{
  return sizeof(struct ottery_config);
}

size_t
ottery_get_sizeof_state(void)
{
  return sizeof(struct ottery_state);
}

#ifndef OTTERY_NO_CLEAR_AFTER_YIELD
/** Used to zero out the contents of our buffer after we've just given a few
 * to the user. */
#define CLEARBUF(ptr,n) do { memset((ptr), 0, (n)); } while (0)
#else
#define CLEARBUF(ptr,n) ((void)0)
#endif

/**
 * Volatile pointer to memset: we use this to keep the compiler from
 * eliminating our call to memset.
 */
void * (*volatile memset_volatile)(void *, int, size_t) = memset;

/**
 * Clear all bytes stored in a structure. Unlike memset, the compiler is not
 * going to optimize this out of existence because the target is about to go
 * out of scope.
 *
 * @param mem Pointer to the memory to erase.
 * @param len The number of bytes to erase.
 */
/* NOTE: whenever we change this, change test/test_memclear.c accordingly */
static void
ottery_memclear_(void *mem, size_t len)
{
  memset_volatile(mem, 0, len);
}

#ifndef OTTERY_NO_WIPE_STACK

/* Chosen more or less arbitrarily */
#define WIPE_STACK_LEN 512

/**
 * Try to clear memory on the stack to clean up after our PRF. This can't
 * easily be done in standard C, so we're doing an ugly hack in hopes that it
 * actually helps.
 *
 * This should never be necessary in a correct program, but if your program is
 * doing something stupid like leaking uninitialized stack, it might keep an
 * attacker from exploiting that.
 **/
static void
ottery_wipe_stack_(void)
{
  char buf[WIPE_STACK_LEN];
  memset_volatile(buf, 0, sizeof(buf));
}
#else
#define ottery_wipe_stack_() ((void)0)
#endif

#if defined(OTTERY_PTHREADS)
/** Acquire the lock for the state "st". */
#define LOCK(st) do {                 \
    pthread_mutex_lock(&(st)->mutex); \
} while (0)
/** Release the lock for the state "st". */
#define UNLOCK(st) do {                 \
    pthread_mutex_unlock(&(st)->mutex); \
} while (0)
#elif defined(OTTERY_CRITICAL_SECTION)
#define LOCK(st) do {                   \
    EnterCriticalSection(&(st)->mutex); \
} while (0)
#define UNLOCK(st) do {                 \
    LeaveCriticalSection(&(st)->mutex); \
} while (0)
#elif defined(OTTERY_OSATOMIC)
#define LOCK(st) do {             \
    OSSpinLockLock(&(st)->mutex); \
} while (0)
#define UNLOCK(st) do {             \
    OSSpinLockUnlock(&(st)->mutex); \
} while (0)
#elif defined(OTTERY_NO_LOCKS)
#define LOCK(st) ((void)0)
#define UNLOCK(st) ((void)0)
#else
#error How do I lock?
#endif

/** Which PRF should we use by default? */
#define OTTERY_PRF_DEFAULT ottery_prf_chacha20_

int
ottery_config_init(struct ottery_config *cfg)
{
  cfg->impl = &OTTERY_PRF_DEFAULT;
  cfg->urandom_fname = NULL;
  return 0;
}

int
ottery_config_force_implementation(struct ottery_config *cfg,
                                   const char *impl)
{
  int i;
  static const struct {
    const char *name;
    const struct ottery_prf *prf;
  } prf_table[] = {
    { OTTERY_PRF_CHACHA,   &ottery_prf_chacha20_, },
    { OTTERY_PRF_CHACHA8,  &ottery_prf_chacha8_, },
    { OTTERY_PRF_CHACHA12, &ottery_prf_chacha12_, },
    { OTTERY_PRF_CHACHA20, &ottery_prf_chacha20_, },

#ifdef OTTERY_HAVE_SIMD_IMPL
    { OTTERY_PRF_CHACHA_SIMD,   &ottery_prf_chacha20_krovetz_, },
    { OTTERY_PRF_CHACHA8_SIMD,  &ottery_prf_chacha8_krovetz_, },
    { OTTERY_PRF_CHACHA12_SIMD, &ottery_prf_chacha12_krovetz_, },
    { OTTERY_PRF_CHACHA20_SIMD, &ottery_prf_chacha20_krovetz_, },
#endif

    { OTTERY_PRF_CHACHA_NO_SIMD,   &ottery_prf_chacha20_merged_, },
    { OTTERY_PRF_CHACHA8_NO_SIMD,  &ottery_prf_chacha8_merged_, },
    { OTTERY_PRF_CHACHA12_NO_SIMD, &ottery_prf_chacha12_merged_, },
    { OTTERY_PRF_CHACHA20_NO_SIMD, &ottery_prf_chacha20_merged_, },

    { NULL, NULL }
  };
  if (!impl)
    return OTTERY_ERR_INVALID_ARGUMENT;
  for (i = 0; prf_table[i].name; ++i) {
    if (0 == strcmp(impl, prf_table[i].name)) {
      cfg->impl = prf_table[i].prf;
      return 0;
    }
  }
  return OTTERY_ERR_INVALID_ARGUMENT;
}

void
ottery_config_set_manual_prf_(struct ottery_config *cfg,
                              const struct ottery_prf *prf)
{
  cfg->impl = prf;
}

void
ottery_config_set_urandom_device_(struct ottery_config *cfg,
                                  const char *fname)
{
  cfg->urandom_fname = fname;
}

/**
 * As ottery_st_nextblock_nolock(), but fill the entire block with
 * entropy, and don't try to rekey the state.
 */
static void
ottery_st_nextblock_nolock_norekey(struct ottery_state *st)
{
  st->prf.generate(st->state, st->buffer, st->block_counter);
  ottery_wipe_stack_();
  ++st->block_counter;
}

/**
 * Generate (st->output_len) bytes of pseudorandom data from the PRF into
 * (st->buffer).  Use the first st->prf.state_bytes of those bytes to replace
 * the PRF state and advance (st->pos) to point after them.
 *
 * This function does not acquire the lock on the state; use it within
 * another function that does.
 *
 * @param st The state to use when generating the block.
 */
static void
ottery_st_nextblock_nolock(struct ottery_state *st)
{
  ottery_st_nextblock_nolock_norekey(st);
  st->prf.setup(st->state, st->buffer);
  CLEARBUF(st->buffer, st->prf.state_bytes);
  st->block_counter = 0;
  st->pos = st->prf.state_bytes;
}

/**
 * Initialize or reinitialize a PRNG state.
 *
 * @param st The state to initialize or reinitialize.
 * @param prf The configuration to use. (Ignored for reinit)
 * @return An OTTERY_ERR_* value (zero on success, nonzero on failure).
 */
static int
ottery_st_initialize(struct ottery_state *st,
                     const struct ottery_config *config,
                     int reinit)
{
  const struct ottery_prf *prf;
  if (!reinit) {
    const char *urandom_fname;
    /* We really need our state to be aligned. If it isn't, let's give an
     * error now, and not a crash when the SIMD instructions start to fail.
     */
    if (((uintptr_t)st) & 0xf)
      return OTTERY_ERR_STATE_ALIGNMENT;

    if (config) {
      urandom_fname = config->urandom_fname;
      prf = config->impl;
    } else {
      prf = &OTTERY_PRF_DEFAULT;
      urandom_fname = NULL;
    }

    memset(st, 0, sizeof(*st));

    /* Now set up the spinlock or mutex or hybrid thing. */
#ifdef OTTERY_PTHREADS
    if (pthread_mutex_init(&st->mutex, NULL))
      return OTTERY_ERR_LOCK_INIT;
#elif defined(OTTERY_CRITICAL_SECTION)
    if (InitializeCriticalSectionAndSpinCount(&st->mutex, 3000) == 0)
      return OTTERY_ERR_LOCK_INIT;
#elif defined(OTTERY_OSATOMIC)
    /* Setting an OSAtomic spinlock to 0 is all you need to do to
     * initialize it.*/
#endif

    /* Check invariants for PRF, in case we wrote some bad code. */
    if ((prf->state_len > MAX_STATE_LEN) ||
        (prf->state_bytes > MAX_STATE_BYTES) ||
        (prf->state_bytes > prf->output_len) ||
        (prf->output_len > MAX_OUTPUT_LEN))
      return OTTERY_ERR_INTERNAL;

    /* Check whether some of our structure size assumptions are right. */
    if ((sizeof(struct ottery_state) > OTTERY_STATE_DUMMY_SIZE_) ||
        (sizeof(struct ottery_config) > OTTERY_CONFIG_DUMMY_SIZE_))
      return OTTERY_ERR_INTERNAL;

    st->urandom_fname = urandom_fname;

    /* Copy the PRF into place. */
    memcpy(&st->prf, prf, sizeof(*prf));
  } else {
    prf = &st->prf;
  }
  /* Now seed the PRF: Generate some random bytes from the OS, and use them
   * as whatever keys/nonces/whatever the PRF wants to have. */
  int err;
  if ((err = ottery_os_randbytes_(st->urandom_fname,
                                  st->buffer, prf->state_bytes)))
    return err;
  prf->setup(st->state, st->buffer);

  /* Generate the first block of output. */
  st->block_counter = 0;
  ottery_st_nextblock_nolock(st);

  st->pid = getpid();

  /* Set the magic number last, or else we might look like we succeeded
   * when we didn't */
  st->magic = MAGIC(st);

  return 0;
}

int
ottery_st_init(struct ottery_state *st, const struct ottery_config *cfg)
{
  return ottery_st_initialize(st, cfg, 0);
}

int
ottery_st_add_seed(struct ottery_state *st, const uint8_t *seed, size_t n)
{
#ifndef OTTERY_NO_INIT_CHECK
  if (UNLIKELY(st->magic != MAGIC(st))) {
    ottery_fatal_error_(OTTERY_ERR_STATE_INIT);
    return OTTERY_ERR_STATE_INIT;
  }
#endif

  /* If the user passed NULL, then we should reseed from the operating
   * system. */
  uint8_t tmp_seed[MAX_STATE_BYTES];

  if (!seed || !n) {
    unsigned state_bytes;
    const char *urandom_fname;
    /* Hold the lock for only a moment here: we don't want to be holding
     * it while we call the OS RNG. */
    LOCK(st);
    state_bytes = st->prf.state_bytes;
    urandom_fname = st->urandom_fname;
    UNLOCK(st);
    int r;
    if ((r = ottery_os_randbytes_(urandom_fname, tmp_seed, state_bytes)))
      return r;
    seed = tmp_seed;
    n = state_bytes;
  }

  LOCK(st);
  /* The algorithm here is really easy. We grab a block of output from the
   * PRNG, that the first (state_bytes) bytes of that, XOR it with up to
   * (state_bytes) bytes of our new seed data, and use that to set our new
   * state. We do this over and over until we have no more seed data to add.
   */
  while (n) {
    unsigned i;
    size_t m = n > st->prf.state_bytes ? st->prf.state_bytes : n;
    ottery_st_nextblock_nolock_norekey(st);
    for (i = 0; i < m; ++i) {
      st->buffer[i] ^= seed[i];
    }
    st->prf.setup(st->state, st->buffer);
    st->block_counter = 0;
    n -= m;
    seed += m;
  }

  /* Now make sure that st->buffer is set up with the new state. */
  ottery_st_nextblock_nolock(st);

  UNLOCK(st);

  /* If we used stack-allocated seed material, wipe it. */
  ottery_memclear_(tmp_seed, sizeof(tmp_seed));

  return 0;
}

void
ottery_st_wipe(struct ottery_state *st)
{
#ifdef OTTERY_PTHREADS
  pthread_mutex_destroy(&st->mutex);
#elif defined(OTTERY_CRITICAL_SECTION)
  DeleteCriticalSection(&st->mutex);
#elif defined(OTTERY_OSATOMIC)
  /* You don't need to do anything to tear down an OSAtomic spinlock. */
#endif

  ottery_memclear_(st, sizeof(struct ottery_state));
}

/**
 * As ottery_st_stir, but do not acquire a lock on the state.
 *
 * @param st The state to stir.
 */
static void
ottery_st_stir_nolock(struct ottery_state *st)
{
#ifdef OTTERY_NO_CLEAR_AFTER_YIELD
  memset(st->buffer, 0, st->pos);
#else
  (void)st;
#endif
}


void
ottery_st_stir(struct ottery_state *st)
{
  LOCK(st);
  ottery_st_stir_nolock(st);
  UNLOCK(st);
}

/** Function that's invoked on a fatal error. See
 * ottery_set_fatal_handler() for more information. */
static void (*ottery_fatal_handler)(int) = NULL;

void
ottery_fatal_error_(int error)
{
  if (ottery_fatal_handler)
    ottery_fatal_handler(error);
  else
    abort();
}

void
ottery_set_fatal_handler(void (*fn)(int))
{
  ottery_fatal_handler = fn;
}

/**
 * Shared prologue for functions generating random bytes from an
 * ottery_state.  Make sure that the state is initialized, lock it, and
 * reseed it (if the PID has changed).
 */
static inline int
ottery_st_rand_lock_and_check(struct ottery_state *st)
{
#ifndef OTTERY_NO_INIT_CHECK
  if (UNLIKELY(st->magic != MAGIC(st))) {
    ottery_fatal_error_(OTTERY_ERR_STATE_INIT);
    return -1;
  }
#endif

  LOCK(st);
#ifndef OTTERY_NO_PID_CHECK
  if (UNLIKELY(st->pid != getpid())) {
    int err;
    if ((err = ottery_st_initialize(st, NULL, 1))) {
      ottery_fatal_error_(OTTERY_ERR_FLAG_POSTFORK_RESEED|err);
      UNLOCK(st);
      return -1;
    }
  }
#endif
  return 0;
}

/**
 * Generate a small-ish number of bytes from an ottery_state, using
 * buffered data.  If there is insufficient data in the buffer right now,
 * use what we have, and generate more.
 *
 * @param st The state to use.
 * @param out A location to write to.
 * @param n The number of bytes to write. Must not be greater than
 *     st->prf.output_len*2 - st->prf.state_bytes - st->pos - 1.
 */
static inline void
ottery_st_rand_bytes_from_buf(struct ottery_state *st, uint8_t *out,
                              size_t n)
{
  if (n + st->pos < st->prf.output_len) {
    memcpy(out, st->buffer+st->pos, n);
    CLEARBUF(st->buffer+st->pos, n);
    st->pos += n;
  } else {
    unsigned cpy = st->prf.output_len - st->pos;
    memcpy(out, st->buffer+st->pos, cpy);
    n -= cpy;
    out += cpy;
    ottery_st_nextblock_nolock(st);
    memcpy(out, st->buffer+st->pos, n);
    CLEARBUF(st->buffer, n);
    st->pos += n;
    assert(st->pos < st->prf.output_len);
  }
}

void
ottery_st_rand_bytes(struct ottery_state *st, void *out_,
                     size_t n)
{
  if (ottery_st_rand_lock_and_check(st))
    return;

  uint8_t *out = out_;
  size_t cpy;

  if (n + st->pos < st->prf.output_len * 2 - st->prf.state_bytes - 1) {
    /* Fulfill it all from the buffer simply if possible. */
    ottery_st_rand_bytes_from_buf(st, out, n);
    UNLOCK(st);
    return;
  }

  /* Okay. That's not going to happen.  Well, take what we can... */
  cpy = st->prf.output_len - st->pos;
  memcpy(out, st->buffer + st->pos, cpy);
  out += cpy;
  n -= cpy;

  /* Then take whole blocks so long as we need them, without stirring... */
  while (n >= st->prf.output_len) {
    /* (We could save a memcpy here if we generated the block directly at out
     * rather than doing the memcpy here. First we'd need to make sure that we
     * had gotten the block aligned to a 16-byte boundary, though, and we'd
     * have some other tricky bookkeeping to do. Let's call this good enough
     * for now.) */
    ottery_st_nextblock_nolock_norekey(st);
    memcpy(out, st->buffer, st->prf.output_len);
    out += st->prf.output_len;
    n -= st->prf.output_len;
  }

  /* Then stir for the last part. */
  ottery_st_nextblock_nolock(st);
  ottery_st_rand_bytes_from_buf(st, out, n);

  UNLOCK(st);
}

/**
 * Assign an integer type from bytes at a possibly unaligned pointer.
 *
 * @param type the type of integer to assign.
 * @param r the integer lvalue to write to.
 * @param p a pointer to the bytes to read from.
 **/
#define INT_ASSIGN_PTR(type, r, p) do { \
    memcpy(&r, p, sizeof(type));        \
} while (0)

/**
 * Shared code for implementing rand_unsigned() and rand_uint64().
 *
 * @param st The state to use.
 * @param inttype The type of integer to generate.
 **/
#define OTTERY_RETURN_RAND_INTTYPE(st, inttype) do {                   \
    if (ottery_st_rand_lock_and_check(st))                             \
      return (inttype)0;                                               \
    inttype result;                                                    \
    if (sizeof(inttype) + (st)->pos <= (st)->prf.output_len) {         \
      INT_ASSIGN_PTR(inttype, result, (st)->buffer + (st)->pos);       \
      CLEARBUF((st)->buffer + (st)->pos, sizeof(inttype));             \
      (st)->pos += sizeof(inttype);                                    \
      if (st->pos == (st)->prf.output_len) {                           \
        ottery_st_nextblock_nolock(st);                                \
      }                                                                \
    } else {                                                           \
      /* Our handling of this case here is significantly simpler */    \
      /* than that of ottery_st_rand_bytes_from_buf, at the expense */ \
      /* of wasting up to sizeof(inttype)-1 bytes. Since inttype */    \
      /* is at most 8 bytes long, that's not such a big deal. */       \
      ottery_st_nextblock_nolock(st);                                  \
      INT_ASSIGN_PTR(inttype, result, (st)->buffer + (st)->pos);       \
      CLEARBUF((st)->buffer, sizeof(inttype));                         \
      (st)->pos += sizeof(inttype);                                    \
    }                                                                  \
    UNLOCK(st);                                                        \
    return result;                                                     \
} while (0)

unsigned
ottery_st_rand_unsigned(struct ottery_state *st)
{
  OTTERY_RETURN_RAND_INTTYPE(st, unsigned);
}

uint32_t
ottery_st_rand_uint32(struct ottery_state *st)
{
  OTTERY_RETURN_RAND_INTTYPE(st, uint32_t);
}

uint64_t
ottery_st_rand_uint64(struct ottery_state *st)
{
  OTTERY_RETURN_RAND_INTTYPE(st, uint64_t);
}

unsigned
ottery_st_rand_range(struct ottery_state *st, unsigned upper)
{
  unsigned lim = upper+1;
  unsigned divisor = lim ? (UINT_MAX / lim) : 1;
  unsigned n;
  do {
    n = (ottery_st_rand_unsigned(st) / divisor);
  } while (n > upper);

  return n;
}

uint64_t
ottery_st_rand_range64(struct ottery_state *st, uint64_t upper)
{
  uint64_t lim = upper+1;
  uint64_t divisor = lim ? (UINT64_MAX / lim) : 1;
  uint64_t n;
  do {
    n = (ottery_st_rand_uint64(st) / divisor);
  } while (n > upper);

  return n;
}
