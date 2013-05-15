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
                uint32_t skip_bytes)
{
  __attribute__((aligned(16))) uint8_t buf[MAX_OUTPUT_LEN];

  uint32_t counter = skip_bytes / stream->prf.output_len;
  skip_bytes -= counter * stream->prf.output_len;
  while (output_len) {
    stream->prf.generate(stream->state, buf, counter);
    ++counter;
    if (stream->prf.output_len - skip_bytes > output_len) {
      memcpy(output, buf + skip_bytes, output_len - skip_bytes);
      output_len = 0;
    } else {
      memcpy(output, buf + skip_bytes , stream->prf.output_len - skip_bytes);
      output_len -= stream->prf.output_len - skip_bytes;
      output += stream->prf.output_len - skip_bytes;
      skip_bytes = 0;
    }
  }
}
