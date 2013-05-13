#include "ottery-internal.h"

#define u64 uint64_t
#define u32 uint32_t
#define u8 uint8_t

#define CHACHA_RNDS 12

#if ! defined(OTTERY_NO_VECS)          \
       && (defined(__ARM_NEON__) ||    \
           defined(__ALTIVEC__)  ||    \
           defined(__SSE2__))
#include "chacha_krovetz.c"
#endif

#include "chacha_merged.c"

