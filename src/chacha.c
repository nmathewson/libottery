#include "chacha.h"
#include <string.h>

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

#endif
