#ifndef OTTERY_ST_H_HEADER_INCLUDED_
#define OTTERY_ST_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

/** @file */

struct ottery_config;

#ifndef OTTERY_INTERNAL
/**
 * The state for a libottery PRNG.
 *
 * An ottery_state structure is constucted with ottery_st_init().  It MUST be
 * aligned on a 16-byte boundary.
 *
 * You may not use an ottery_state structure with any other function before
 * you have first initialized it with ottery_st_init().
 *
 * The contents of this structure are opaque; The definition here is
 * defined to be large enough so that programs that allocate it will get
 * more than enough room.
 */
struct __attribute__((aligned(16))) ottery_state {
  /** Nothing to see here */
  uint8_t dummy_[1024];
};
#endif

/**
 * Initialize an ottery_state structure.
 *
 * You must call this function on any ottery_state structure before
 * calling any other functions on it.
 *
 * @param st The ottery_state to initialize.
 * @param cfg Either NULL, or an ottery_config structure that has been
 *   initialized with ottery_config_init().
 * @return Zero on success, or one of the OTTERY_ERR_* error codes on failure.
 */
int ottery_st_init(struct ottery_state *st, const struct ottery_config *cfg);

/**
 * Add more entropy to an ottery_state structure.
 *
 * Calling this function should be needless, if you trust your operating
 * system's random number generator and entropy extraction features.  You
 * would want to use this function if you think the operating system's random
 * number generator might be inadequate, and you want to add more entropy from
 * EGD or something.
 *
 * You might also want to call this function if your belief system says that
 * it's useful to periodically add more raw entropy to a well-seeded
 * cryptographically strong PRNG.
 *
 * @param st The state which will receive more entropy.
 * @param seed Bytes to add to the state.
 * @param n The number of bytes to add.
 */
void ottery_st_add_seed(struct ottery_state *st, const uint8_t *seed, size_t n);

/**
 * Destroy an ottery_state structure and release any resources that it might
 * hold.
 *
 * Ordinarily, you would want to call this at exit, or before freeing an
 * ottery_state
 *
 * @param st The state to wipet.
 */
void ottery_st_wipe(struct ottery_state *st);

/**
 * Explicitly "stir" an ottery_state structure.
 *
 * Once the state has been stirred, an attacker who compromises the state
 * later on will not be able to recover bytes that have previously been
 * returned by any of the ottery_st_rand_* functions.
 *
 * You shouldn't need to call this function in ordinary use; libottery
 * self-stirs every so often.
 *
 * @param st The state to stir.
 */
void ottery_st_stir(struct ottery_state *st);

/**
 * Use an ottery_state structure to fill a buffer with random bytes.
 *
 * @param st The state structure to use.
 * @param buf The buffer to fill.
 * @param n The number of bytes to write.
 */
void ottery_st_rand_bytes(struct ottery_state *st, void *out, size_t n);
/**
 * Use an ottery_state structure to generate a random number of type unsigned.
 *
 * @param st The state structure to use.
 * @return A random number between 0 and UINT_MAX included,
 *   chosen uniformly.
 */
unsigned ottery_st_rand_unsigned(struct ottery_state *st);
/**
 * Use an ottery_state structure to generate a random number of type uint64_t.
 *
 * @return A random number between 0 and UINT64_MAX included,
 *   chosen uniformly.
 */
uint64_t ottery_st_rand_uint64(struct ottery_state *st);
/**
 * Use an ottery_state structure to generate a random number of type unsigned
 * in a given range.
 *
 * @param top The upper bound of the range (inclusive).
 * @return A random number no larger than top, and no less than 0,
 *   chosen uniformly.
 */
unsigned ottery_st_rand_range(struct ottery_state *st, unsigned top);
/**
 * Use an ottery_state structure to generate a random number of type uint64_t
 * in a given range.
 *
 * @param top The upper bound of the range (inclusive).
 * @return A random number no larger than top, and no less than 0,
 *   chosen uniformly.
 */
uint64_t ottery_st_rand_range64(struct ottery_state *st, uint64_t top);

#endif
