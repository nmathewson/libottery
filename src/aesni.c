// Based on AESNI code from rijndael.cpp from Crypto++. The notice there is as
// follows, though I'm not sure how much applies to what we retain here.

// rijndael.cpp - modified by Chris Morgan <cmorgan@wpi.edu>
// and Wei Dai from Paulo Baretto's Rijndael implementation
// The original code and all modifications are in the public domain.

#ifndef __SSE2__
#error "No SSE2 support."
#endif

#include "ottery-internal.h"
#include <string.h>
#include <assert.h>

#ifdef __AES__
#include <wmmintrin.h>
#include <smmintrin.h>
#else
#include <smmintrin.h>

#define VERY_INLINE(rettype) \
  static inline rettype __attribute__((__always_inline__, __artificial__))

VERY_INLINE(__m128i)
_mm_aeskeygenassist_si128 (__m128i a, const int i)
{
        __m128i r;
        asm ("aeskeygenassist %2, %1, %0" : "=x"(r) : "xm"(a), "i"(i));
        return r;
}
VERY_INLINE(__m128i)
_mm_aesenc_si128 (__m128i a, __m128i b)
{
        asm ("aesenc %1, %0" : "+x"(a) : "xm"(b));
        return a;
}
VERY_INLINE(__m128i)
_mm_aesenclast_si128 (__m128i a, __m128i b)
{
        asm ("aesenclast %1, %0" : "+x"(a) : "xm"(b));
        return a;
}
#endif

#define ALIGNED __attribute__((aligned(16)))

#define NONCELEN 12

#if KEYLEN == 32
#define NAME "AES-256"
#define ROUNDS 14
#define ottery_prf_aesni ottery_prf_aes256_
#elif KEYLEN == 24
#define NAME "AES-192"
#define ROUNDS 12
#define ottery_prf_aesni ottery_prf_aes192_
#elif KEYLEN == 16
#define NAME "AES-128"
#define ROUNDS 10
#define ottery_prf_aesni ottery_prf_aes128_
#else
#error "weird keylen."
#endif

struct ALIGNED aesni_state {
  ALIGNED __m128i subkeys[ROUNDS+1];
  ALIGNED __m128i nonce;
};

static void
set_key(struct aesni_state *st, const uint8_t *userKey, unsigned int keylen)
{
  static const uint32_t rcLE[] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80,
    0x1B, 0x36, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
  };
  const uint32_t *rc = rcLE;
  uint32_t *rk = (uint32_t *)st->subkeys;

  assert(keylen == KEYLEN);

  __m128i temp = _mm_loadu_si128((__m128i *)(userKey+keylen-16));
  memcpy(rk, userKey, keylen);

  while (1)
  {
    rk[keylen/4] = rk[0] ^ _mm_extract_epi32(_mm_aeskeygenassist_si128(temp, 0), 3) ^ *(rc++);
    rk[keylen/4+1] = rk[1] ^ rk[keylen/4];
    rk[keylen/4+2] = rk[2] ^ rk[keylen/4+1];
    rk[keylen/4+3] = rk[3] ^ rk[keylen/4+2];

    if (keylen/4 + 3 == ROUNDS)
      break;

    if (keylen == 24)
    {
      rk[10] = rk[ 4] ^ rk[ 9];
      rk[11] = rk[ 5] ^ rk[10];
      temp = _mm_insert_epi32(temp, rk[11], 3);
    }
    else if (keylen == 32)
    {
      temp = _mm_insert_epi32(temp, rk[11], 3);
      rk[12] = rk[ 4] ^ _mm_extract_epi32(_mm_aeskeygenassist_si128(temp, 0), 2);
      rk[13] = rk[ 5] ^ rk[12];
      rk[14] = rk[ 6] ^ rk[13];
      rk[15] = rk[ 7] ^ rk[14];
      temp = _mm_insert_epi32(temp, rk[15], 3);
    }
    else
      temp = _mm_insert_epi32(temp, rk[7], 3);

    rk += keylen/4;
  }
}

static void
aesni_generate(void *state_, uint8_t *output, uint32_t idx)
{
  struct aesni_state *state = state_;
  /* XXXX is 3 right???? */
  __m128i block0 = _mm_insert_epi32(state->nonce, idx, 3);
  __m128i block1 = _mm_insert_epi32(state->nonce, idx+1, 3);
  __m128i block2 = _mm_insert_epi32(state->nonce, idx+2, 3);
  __m128i block3 = _mm_insert_epi32(state->nonce, idx+3, 3);
  __m128i *subkeys = state->subkeys;

  __m128i rk = subkeys[0];
  block0 = _mm_xor_si128(block0, rk);
  block1 = _mm_xor_si128(block1, rk);
  block2 = _mm_xor_si128(block2, rk);
  block3 = _mm_xor_si128(block3, rk);
  unsigned int i;
  for (i=1; i<ROUNDS; i++)
  {
    rk = subkeys[i];
    block0 = _mm_aesenc_si128(block0, rk);
    block1 = _mm_aesenc_si128(block1, rk);
    block2 = _mm_aesenc_si128(block2, rk);
    block3 = _mm_aesenc_si128(block3, rk);
  }
  rk = subkeys[ROUNDS];
  block0 = _mm_aesenclast_si128(block0, rk);
  block1 = _mm_aesenclast_si128(block1, rk);
  block2 = _mm_aesenclast_si128(block2, rk);
  block3 = _mm_aesenclast_si128(block3, rk);

  __m128i *out = (__m128i*)output;
  out[0] = block0;
  out[1] = block1;
  out[2] = block2;
  out[3] = block3;
}

static void
aesni_state_setup(void *state_, const uint8_t *bytes)
{
  struct aesni_state *state = state_;
  set_key(state, bytes, KEYLEN);
  memset(&state->nonce, 0, sizeof(state->nonce));
  /* XXXX Can I use NONCE this way? */
  memcpy(&state->nonce, bytes+KEYLEN, NONCELEN);
}

#define STATE_LEN (sizeof(struct aesni_state))
#define STATE_BYTES (KEYLEN + NONCELEN)
#define OUTPUT_LEN (64)
#define IDX_STEP 4

const struct ottery_prf ottery_prf_aesni = {
  NAME,
  "aesni",
  STATE_LEN,
  STATE_BYTES,
  OUTPUT_LEN,
  IDX_STEP,
  aesni_state_setup,
  aesni_generate,
};

