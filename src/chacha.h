#ifndef CHACHA_H_HEADER_INCLUDED_
#define CHACHA_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

struct chacha_state {
  uint8_t key[32];
  uint8_t nonce[8];
  uint64_t block_counter;
};

void chacha_state_setup(struct chacha_state *state,
                        const uint8_t *key,
                        const uint8_t *nonce,
                        uint64_t counter);

int chacharand_os_bytes(uint8_t *bytes, size_t n);

int crypto_stream_8(uint8_t *out,
                     uint64_t output_len,
                     struct chacha_state *st);
int crypto_stream_20(uint8_t *out,
                     uint64_t output_len,
                     struct chacha_state *st);

#endif
