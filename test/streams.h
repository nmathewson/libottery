/* Libottery by Nick Mathewson.

   This software has been dedicated to the public domain under the CC0
   public domain dedication.

   To the extent possible under law, the person who associated CC0 with
   libottery has waived all copyright and related or neighboring rights
   to libottery.

   You should have received a copy of the CC0 legalcode along with this
   work in doc/cc0.txt.  If not, see
      <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
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
