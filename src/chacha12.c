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
#include "ottery-internal.h"

#define u64 uint64_t
#define u32 uint32_t
#define u8 uint8_t

#define CHACHA_RNDS 12

#if !defined(OTTERY_NO_VECS)   \
  && (defined(__ARM_NEON__) || \
  defined(__ALTIVEC__)  ||     \
  defined(__SSE2__))
#include "chacha_krovetz.c"
#endif

#include "chacha_merged.c"

