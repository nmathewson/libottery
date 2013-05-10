#include "ottery-internal.h"
#include <string.h>/* XXXX remove me.*/

#define u64 uint64_t
#define u32 uint32_t
#define u8 uint8_t

#define CHACHA_RNDS 8

#if ! defined(OTTERY_NO_VECS)          \
       && (defined(__ARM_NEON__) ||    \
           defined(__ALTIVEC__)  ||    \
           defined(__SSE2__))
#include "src/chacha_krovetz.c"
#else
#include "src/chacha_merged.c"
#endif

/* XXXX hack; this API should not be. */
void
ottery_chacha_state_setup_(struct chacha_state *state,
                             const uint8_t *key,
                             const uint8_t *nonce,
                             uint64_t counter)
{
  memcpy(state->key, key, 32);
  memcpy(state->nonce, nonce, 8);
  state->block_counter = counter;
}
