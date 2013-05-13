#ifndef STREAMS_H_HEADER_INCLUDED_
#define STREAMS_H_HEADER_INCLUDED_

/* wrap the PRF functions to implement a stream generator. */
#include "ottery-internal.h"

struct stream {
  __attribute__((aligned(16))) uint8_t state[MAX_STATE_LEN];
  struct ottery_prf prf;
};

void stream_setup(struct stream *stream,
                  const struct ottery_prf *prf,
                  const uint8_t *key, size_t keylen,
                  const uint8_t *nonce, size_t noncelen);

void stream_generate(struct stream *st,
                     uint8_t *output,
                     size_t output_len,
                     uint32_t skip_bytes);
#endif
