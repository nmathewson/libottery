#include "streams.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>

void
stream_setup(struct stream *stream,
             const struct ottery_prf *prf,
             const uint8_t *key, size_t keylen,
             const uint8_t *nonce, size_t noncelen)
{
  assert(keylen + noncelen == prf->state_bytes);
  uint8_t input[MAX_STATE_BYTES];
  memcpy(input, key, keylen);
  memcpy(input+keylen, nonce, noncelen);

  prf->setup(stream->state, input);
  memcpy(&stream->prf, prf, sizeof(stream->prf));
}

void
stream_generate(struct stream *stream,
                uint8_t *output,
                size_t output_len,
                uint32_t skip_blocks)
{
  __attribute__((aligned(16))) uint8_t buf[MAX_OUTPUT_LEN];
  uint32_t counter = skip_blocks;
  while (output_len) {
    stream->prf.generate(stream->state, buf, counter);
    counter += stream->prf.idx_step;
    if (stream->prf.output_len > output_len) {
      memcpy(output, buf, output_len);
      output_len = 0;
    } else {
      memcpy(output, buf, stream->prf.output_len);
      output_len -= stream->prf.output_len;
      output += stream->prf.output_len;
    }
  }
}
