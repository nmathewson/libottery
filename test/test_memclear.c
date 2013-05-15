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

/* This is a horrible hack to try to see whether ottery_memclear_ successfully
 * avoids getting eliminated.  Modern compilers have a fun tendency to refuse
 * to clear memory memset()s when the memory is about to go out of scope. */

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define N 16384
const char buf[16] = "What's a Ftumch?";
#define TOTAL 1430528

static inline void ottery_memclear_(void *mem, size_t len)
  __attribute__((always_inline));

/* I'm copying this here so that it has the best chance of getting inlined. */
static inline void
ottery_memclear_(void *mem, size_t len)
{
  volatile uint8_t *cp = mem;
  while (len--)
    *cp++ = 0;
}

/* This function exists mainly to fill the stack up with a predictable
 * pattern, and then clear the pattern as it returns.  The calculations are
 * there to keep the memcpys from getting eliminated. */
uint32_t
f1(void)
{
  int i;
  uint8_t mem[N];
  uint32_t r = 0;
  for (i=0; i<N; i += 16) {
    memcpy(mem+i, buf, 16);
  }
  for (i=0; i<N; ++i) {
    r += mem[i];
  }
  ottery_memclear_(mem, sizeof(mem));
  return r;
}

/* Now, we get another buffer in more or less the same place, and check for
 * the pattern we made. Here we're looking at an uninitialized buffer, which
 * technically counts as undefined behavior. */
int
f2(void)
{
  int i;
  uint8_t mem[N];
  for (i=0;i<N-16;++i) {
    if (!memcmp(mem+i, buf, 16))
      return i;
  }
  return -1;
}

int
main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
  uint32_t u = f1();
  int found = f2();
  if (found == -1 && u == TOTAL)
    puts("OKAY");
  else
    puts("PROBLEM");

  return 0;
}
