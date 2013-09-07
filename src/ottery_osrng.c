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
#define OTTERY_INTERNAL
#include "ottery-internal.h"
#include "ottery.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SRC(x) OTTERY_ENTROPY_SRC_ ## x
#define FL(x) OTTERY_ENTROPY_FL_ ## x

#include "ottery_entropy_cryptgenrandom.c"
#include "ottery_entropy_urandom.c"
#include "ottery_entropy_rdrand.c"
#include "ottery_entropy_egd.c"

/** Table of RNG functions and their properties. */
static struct ottery_randbytes_source {
  int (*fn)(const struct ottery_osrng_config *, uint8_t *, size_t);
  uint32_t flags;
} RAND_SOURCES[] = {
#ifdef ENTROPY_SOURCE_CRYPTGENRANDOM
  ENTROPY_SOURCE_CRYPTGENRANDOM,
#endif
#ifdef ENTROPY_SOURCE_URANDOM
  ENTROPY_SOURCE_URANDOM,
#endif
#ifdef ENTROPY_SOURCE_EGD
  ENTROPY_SOURCE_EGD,
#endif
#ifdef ENTROPY_SOURCE_RDRAND
  ENTROPY_SOURCE_RDRAND,
#endif
  { NULL, 0 }
};


int
ottery_os_randbytes_(const struct ottery_osrng_config *config,
                     uint32_t select_sources,
                     uint8_t *bytes, size_t n,
                     uint8_t *scratch, uint32_t *flags_out)
{
  int err = OTTERY_ERR_INIT_STRONG_RNG, i, last_err = 0;
  size_t j;
  uint32_t got = 0;
  const uint32_t disabled_sources = config ? config->disabled_sources : 0;

  memset(bytes, 0, n);

  *flags_out = 0;

  for (i=0; RAND_SOURCES[i].fn; ++i) {
    uint32_t flags = RAND_SOURCES[i].flags;
    /* Don't use a disabled source. */
    if (0 != (flags & disabled_sources))
      continue;
    /* If some flags must be set, only use those. */
    if ((flags & select_sources) != select_sources)
      continue;
    err = RAND_SOURCES[i].fn(config, scratch, n);
    if (err == 0) {
      got |= RAND_SOURCES[i].flags;
      /* XXXXX In theory, we should be combining these sources with a better
       * function than XOR. XOR is good enough if they're independent, though.
       */
      for (j = 0; j < n; ++j) {
        bytes[j] ^= scratch[j];
      }
    } else {
      last_err = err;
    }
  }

  /* Do not report success unless at least one source was strong. */
  if (0 == (got & OTTERY_ENTROPY_FL_STRONG))
    return last_err ? last_err : OTTERY_ERR_INIT_STRONG_RNG;

  /* Wipe the scratch space */
  ottery_memclear_(scratch, n);

  *flags_out = got;
  return 0;
}
