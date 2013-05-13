/** \file ottery-config.h
 *
 * Configuration file for libottery.  The values here affect internal
 * behavior only; not external APIs.
 */
#ifndef OTTERY_CONFIG_H_HEADER_INCLUDED_
#define OTTERY_CONFIG_H_HEADER_INCLUDED_

/**
 * If OTTERY_NO_VECS is defined, disable SIMD-based backends.
 */
/* #undef OTTERY_NO_VECS */

/**
 * If OTTERY_NO_PID_CHECK is defined, do no safety checking for fork().  This
 * can be catastrophic if your program forks and the subprocess might invoke
 * the PRNG.
 */
/* #undef OTTERY_NO_PID_CHECK */

/**
 * If OTTERY_NO_INIT_CHECK is defined, do no checking to make sure that
 * ottery_state structures are initialized and seeded. This can be
 * catastropic if your program does not make absolutely sure to call
 * ottery_init() and ottery_st_init() as appropriate.
 */
/* #undef OTTERY_NO_INIT_CHECK */

/**
 * If OTTERY_NO_LOCKS is defined, don't lock any data structures. This can be
 * catastropic if any ottery_state, including the implicit global
 * ottery_state, is used from multiple threads.
 */
/* #undef OTTERY_NO_LOCKS */

#endif
