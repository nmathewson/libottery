#ifndef OTTERY_INTERNAL_H_HEADER_INCLUDED_
#define OTTERY_INTERNAL_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

int ottery_os_randbytes_(uint8_t *bytes, size_t n);


#if ! defined(OTTERY_NO_VECS)          \
       && (defined(__ARM_NEON__) ||    \
           defined(__ALTIVEC__)  ||    \
           defined(__SSE2__))
#define OTTERY_HAVE_SIMD_IMPL
#endif

#define MAX_STATE_BYTES 64
#define MAX_STATE_LEN 256
#define MAX_OUTPUT_LEN 256

/* REQUIRED: state_bytes <= output_len */

struct ottery_prf {
  unsigned state_len;
  unsigned state_bytes;
  unsigned output_len;
  unsigned idx_step;
  void (*setup)(void *state, const uint8_t *bytes);
  void (*generate)(void *state, uint8_t *output, uint32_t idx);
};

extern const struct ottery_prf ottery_prf_chacha8_merged_;
extern const struct ottery_prf ottery_prf_chacha12_merged_;
extern const struct ottery_prf ottery_prf_chacha20_merged_;

#ifdef OTTERY_HAVE_SIMD_IMPL
extern const struct ottery_prf ottery_prf_chacha8_krovetz_;
extern const struct ottery_prf ottery_prf_chacha12_krovetz_;
extern const struct ottery_prf ottery_prf_chacha20_krovetz_;
#define ottery_prf_chacha8_ ottery_prf_chacha8_krovetz_
#define ottery_prf_chacha12_ ottery_prf_chacha12_krovetz_
#define ottery_prf_chacha20_ ottery_prf_chacha20_krovetz_
#else
#define ottery_prf_chacha8_ ottery_prf_chacha8_merged_
#define ottery_prf_chacha12_ ottery_prf_chacha12_merged_
#define ottery_prf_chacha20_ ottery_prf_chacha20_merged_
#endif

#endif
