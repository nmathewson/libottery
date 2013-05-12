#ifndef OTTERY_INTERNAL_H_HEADER_INCLUDED_
#define OTTERY_INTERNAL_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

int ottery_os_randbytes_(uint8_t *bytes, size_t n);

struct chacha_state {
  __attribute__ ((aligned (16))) uint8_t key[32];
  __attribute__ ((aligned (16))) uint8_t nonce[8];
  uint64_t block_counter;
};
void ottery_chacha_state_setup_(struct chacha_state *state,
                               const uint8_t *key,
                               const uint8_t *nonce,
                               uint64_t counter);

int ottery_stream_chacha8_(uint8_t *out,
                           uint64_t output_len,
                           struct chacha_state *st);
int ottery_stream_chacha20_(uint8_t *out,
                            uint64_t output_len,
                            struct chacha_state *st);

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

extern const struct ottery_prf ottery_prf_chacha8_;
extern const struct ottery_prf ottery_prf_chacha12_;
extern const struct ottery_prf ottery_prf_chacha20_;

#endif
