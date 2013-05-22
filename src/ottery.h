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
#ifndef OTTERY_H_HEADER_INCLUDED_
#define OTTERY_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

/** @file */

struct ottery_config;
struct ottery_state;

/* Functions that use an implicit global state */

/**
 * Fill a buffer with random bytes.
 *
 * @param buf The buffer to fill.
 * @param n The number of bytes to write.
 */
void ottery_rand_bytes(void *buf, size_t n);
/**
 * Generate a random number of type unsigned.
 *
 * @return A random number between 0 and UINT_MAX included,
 *   chosen uniformly.
 */
unsigned ottery_rand_unsigned(void);
/**
 * Generate a random number of type uint64_t.
 *
 * @return A random number between 0 and UINT64_MAX included,
 *   chosen uniformly.
 */
uint64_t ottery_rand_uint64(void);
/**
 * Generate a random number of type unsigned in a given range.
 *
 * @param top The upper bound of the range (inclusive).
 * @return A random number no larger than top, and no less than 0,
 *   chosen uniformly.
 */
unsigned ottery_rand_range(unsigned top);
/**
 * Generate a random number of type uint64_t in a given range.
 *
 * @param top The upper bound of the range (inclusive).
 * @return A random number no larger than top, and no less than 0,
 *   chosen uniformly.
 */
uint64_t ottery_rand_range64(uint64_t top);

/**
 * Initialize the libottery global state.
 *
 * Most users should not need to use this function. If you use it, you must
 * call it before any of: ottery_rand_bytes, ottery_rand_unsigned,
 * ottery_rand_uint64, ottery_rand_range, ottery_rand_uint64_range,
 * ottery_add_seed, ottery_wipe, ottery_stir.
 *
 * You would want to use this function if you want to select some non-default
 * behavior using an ottery_config structure.
 *
 * @param cfg Either NULL, or an ottery_config structure that has been
 *   initialized with ottery_config_init().
 * @return Zero on success, or one of the OTTERY_ERR_* error codes on failure.
 */
int ottery_init(const struct ottery_config *cfg);

/**
 * Add more entropy to the libottery global state.
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
 * @param seed Bytes to add to the state. If this value is NULL, we take
 *    more random bytes from the OS.
 * @param n The number of bytes to add. If this value is 0, we take more
 *    random bytes from the OS, regardless of the value of seed.
 * @return Zero on success, or one of the OTTERY_ERR_* error codes on failure.
 */
int ottery_add_seed(const uint8_t *seed, size_t n);

/**
 * Destroy the libottery global state and release any resources that it might
 * hold.
 *
 * Ordinarily, you would only want to call this at exit, if at all.
 */
void ottery_wipe(void);

/**
 * Explicitly "stir" the libottery global state.
 *
 * Once the state has been stirred, an attacker who compromises the state
 * later on will not be able to recover bytes that have previously been
 * returned by any of the ottery_rand_* functions.
 *
 * Libottery is forward-secure by default, so unless you have manually
 * recompiled libottery with the OTTERY_NO_CLEAR_AFTER_YIELD option, this
 * function isn't necessary and has no effect.  Even *with* the
 * OTTERY_NO_CLEAR_AFTER_YIELD, this function isn't necessary in ordinary
 * operation: the libottery state is implicitly "stirred" every 1k or so.
 */
void ottery_stir(void);

/* Error codes */

/**
 * @name libottery error codes and flags
 *
 * @{
 */
/** No error has occurred. */
#define OTTERY_ERR_NONE                  0x0000
/** We failed to allocate or initialize a lock. */
#define OTTERY_ERR_LOCK_INIT             0x0001
/** An internal error occurrred. This is probably a programming mistake
 * in libottery. */
#define OTTERY_ERR_INTERNAL              0x0002
/** We were unable to connect to the operating system's strong RNG. */
#define OTTERY_ERR_INIT_STRONG_RNG       0x0003
/** We were unable to retrieve sufficient random bytes from the
 * operating system's strong RNG. */
#define OTTERY_ERR_ACCESS_STRONG_RNG     0x0004
/** At least one argument to the function was invalid. */
#define OTTERY_ERR_INVALID_ARGUMENT      0x0005
/** An ottery_state structure was not aligned to a 16-byte boundary. */
#define OTTERY_ERR_STATE_ALIGNMENT       0x0006

/** FATAL ERROR: An ottery_st function other than ottery_st_init() was
 * called on and uninitialized state. */
#define OTTERY_ERR_STATE_INIT            0x1000
/** FLAG; FATAL ERROR: The error occurred while initializing the global
 * state during the first call to an ottery_rand_* function. */
#define OTTERY_ERR_FLAG_GLOBAL_PRNG_INIT 0x2000
/** FLAG; FATAL ERROR: The error occurred while reinitializing a state
 * after a fork().  (We need to do this, or else both processes would
 * generate the same values, which could give dire results.)
 */
#define OTTERY_ERR_FLAG_POSTFORK_RESEED  0x4000

/**
 * Checks whether an OTTERY_ERR value is a fatal error.
 *
 * @param err an OTTERY_ERR_* valuer
 * @return True if err is fatal; false if it is not fatal.
 */
#define OTTERY_ERR_IS_FATAL(err) \
  (((err) & ~0xfff) != 0)

/* Functions to interact with the library on a global level */

/**
 * Override the behavior of libottery on a fatal error.
 *
 * By default, libottery will call abort() in a few circumstances, in
 * order to keep the program from operating insecurely.  If you want,
 * you can provide another function to call instead.
 *
 * If your function does not itself abort() or exit() the process, or throw an
 * exception (assuming some C family that has exceptions), libottery will
 * continue running insecurely -- it might return predictable random numbers,
 * leak secrets, or just return 0 for everything -- so you should really be
 * very careful here.
 *
 * (The alternative to fatal errors would have been having all the
 * ottery_rand_* functions able to return an error, and requiring users
 * to check those codes.  But experience suggests that C programmers
 * frequently do not check error codes.)
 *
 * @param fn A function to call in place of abort(). It will receive as
 *    its argument one of the OTTERY_ERR_* error codes.
 */
void ottery_set_fatal_handler(void (*fn)(int errorcode));

/* Functions to manipulate parameters. */

/**
 * @name Names of prfs for use with ottery_config_force_implementation
 *
 * @{ */
#define OTTERY_PRF_CHACHA   "CHACHA"
#define OTTERY_PRF_CHACHA8  "CHACHA8"
#define OTTERY_PRF_CHACHA12 "CHACHA12"
#define OTTERY_PRF_CHACHA20 "CHACHA20"
#define OTTERY_PRF_CHACHA_SIMD   "CHACHA-SIMD"
#define OTTERY_PRF_CHACHA8_SIMD  "CHACHA8-SIMD"
#define OTTERY_PRF_CHACHA12_SIMD "CHACHA12-SIMD"
#define OTTERY_PRF_CHACHA20_SIMD "CHACHA20-SIMD"
#define OTTERY_PRF_CHACHA_NO_SIMD   "CHACHA-NOSIMD"
#define OTTERY_PRF_CHACHA8_NO_SIMD  "CHACHA8-NOSIMD"
#define OTTERY_PRF_CHACHA12_NO_SIMD "CHACHA12-NOSIMD"
#define OTTERY_PRF_CHACHA20_NO_SIMD "CHACHA20-NOSIMD"
/** @} */

/**
 * Initialize an ottery_config structure.
 *
 * You must call this function on any ottery_config structure before it
 * can be passed to ottery_init() or ottery_st_init().
 *
 * @param cfg The configuration object to initialize.
 * @return Zero on success, or one of the OTTERY_ERR_* error codes on
 *    failure.
 */
int ottery_config_init(struct ottery_config *cfg);

/**
 * Try to force the use of a particular pseudorandom function for a given
 * libottery instance.
 *
 * To use this function, you call it on an ottery_config structure after
 * ottery_config_init(), and before passing that structure to
 * ottery_st_init() or ottery_init().
 *
 * @param cfg The configuration structure to configure.
 * @param impl The name of a pseudorandom function. One of the
 *    OTTERY_PRF_* values.
 * @return Zero on success, or one of the OTTERY_ERR_* error codes on
 *    failure.
 */
int ottery_config_force_implementation(struct ottery_config *cfg,
                                       const char *impl);

/** Size reserved for struct ottery_config */
#define OTTERY_CONFIG_DUMMY_SIZE_ 1024

#ifndef OTTERY_INTERNAL
/**
 * A configuration object for setting up a libottery instance.
 *
 * An ottery_config structure is initialized with ottery_config_init,
 * and passed to ottery_init() or ottery_st_init().
 *
 * The contents of this structure are opaque; The definition here is
 * defined to be large enough so that programs that allocate it will get
 * more than enough room.
 */
struct ottery_config {
  /** Nothing to see here */
  uint8_t dummy_[OTTERY_CONFIG_DUMMY_SIZE_];
};
#endif

/**
 * Get the minimal size for allocating an ottery_config.
 *
 * sizeof(ottery_config) will give an overestimate to allow binary
 * compatibility with future versions of libottery. Use this function instead
 * to get the minimal number of bytes to allocate.
 *
 * @return The minimal number of bytes to use when allocating an
 *   ottery_config structure.
 */
size_t ottery_get_sizeof_config(void);

#endif
