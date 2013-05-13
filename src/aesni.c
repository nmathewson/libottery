
// Based on AESNI code from rijndael.cpp from Crypto++; notice there is as
// follows:

// rijndael.cpp - modified by Chris Morgan <cmorgan@wpi.edu>
// and Wei Dai from Paulo Baretto's Rijndael implementation
// The original code and all modifications are in the public domain.

#ifndef __SSE2__
#error "No SSE2 support."
#endif

#include "ottery-internal.h"

#ifdef __AES__
#include <wmmintrin.h>
#else
#include <emmintrin.h>

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

#define KEYLEN 32

#define NONCELEN 12

#if KEYLEN == 32
#define NAME "AES-256"
#define ROUNDS 14
#elif KEYLEN == 24
#define NAME "AES-192"
#define ROUNDS 12
#elif KEYLEN == 16
#define NAME "AES-128"
#define ROUNDS 10
#else
#error "weird keylen."
#endif

ALIGNED struct aesni_state {
  ALIGNED __m128i subkeys[ROUNDS+1];
  ALIGNED __m1128 nonce;
};

static void
set_key(struct aesni_state *st, const uint8_t *userkey, unsigned int keylen)
{
  static const word32 rcLE[] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80,
    0x1B, 0x36, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
  };
  const word32 *rc = rcLE;

  assert(keylen == KEYLEN);

  __m128i temp = _mm_loadu_si128((__m128i *)(userKey+keylen-16));
  memcpy(rk, userKey, keylen);

  while (true)
  {
    rk[keylen/4] = rk[0] ^ _mm_extract_epi32(_mm_aeskeygenassist_si128(temp, 0), 3) ^ *(rc++);
    rk[keylen/4+1] = rk[1] ^ rk[keylen/4];
    rk[keylen/4+2] = rk[2] ^ rk[keylen/4+1];
    rk[keylen/4+3] = rk[3] ^ rk[keylen/4+2];

    if (rk + keylen/4 + 4 == m_key.end())
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

static inline void AESNI_Enc_4_Blocks(__m128i &block0, __m128i &block1, __m128i &block2, __m128i &block3, const __m128i *subkeys, unsigned int rounds)
{
	__m128i rk = subkeys[0];
	block0 = _mm_xor_si128(block0, rk);
	block1 = _mm_xor_si128(block1, rk);
	block2 = _mm_xor_si128(block2, rk);
	block3 = _mm_xor_si128(block3, rk);
	for (unsigned int i=1; i<rounds; i++)
	{
		rk = subkeys[i];
		block0 = _mm_aesenc_si128(block0, rk);
		block1 = _mm_aesenc_si128(block1, rk);
		block2 = _mm_aesenc_si128(block2, rk);
		block3 = _mm_aesenc_si128(block3, rk);
	}
	rk = subkeys[rounds];
	block0 = _mm_aesenclast_si128(block0, rk);
	block1 = _mm_aesenclast_si128(block1, rk);
	block2 = _mm_aesenclast_si128(block2, rk);
	block3 = _mm_aesenclast_si128(block3, rk);
}

void
aesni_state_setup(void *state_, const uint8_t *bytes)
{
  struct aesni_state *state = state_;
  set_key(state, bytes, KEYLEN);
  memset(&state->nonce, 0, sizeof(state->nonce));
  memcpy(&state->nonce, bytes+KEYLEN, NONCELEN);
}

void
aesni_generate(void *state, uint8_t *output, uint32_t idx)
{
  struct aesni_state *state = state_;
  __m128i *out = (__m128i*)output;
  __m128i b0 = _mm_insert_epi32(state->nonce, 3, idx);
  __m128i b1 = _mm_insert_epi32(state->nonce, 3, idx+1);
  __m128i b2 = _mm_insert_epi32(state->nonce, 3, idx+2);
  __m128i b3 = _mm_insert_epi32(state->nonce, 3, idx+3);

  AESNI_Enc_4_Blocks(&b0, &b1, &b2, &b3, state->subkeys, ROUNDS);
  out[0] = b0;
  out[1] = b1;
  out[2] = b2;
  out[3] = b3;
}

