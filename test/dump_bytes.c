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
#include <unistd.h>
#include "ottery.h"
#include <stdlib.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    if (argc > 1) {
    struct ottery_config cfg;
    const char *implname = argv[1];
    ottery_config_init(&cfg);
    if (ottery_config_force_implementation(&cfg, implname) < 0) {
      printf("Unsupported/unrecognized PRF \"%s\"", argv[1]);
      return 1;
    }
    ottery_init(&cfg);
  }
  while (1) {
    unsigned char buf[8192];
    ottery_rand_bytes(buf, 8192);
    write(1, buf, 8192);
  }

  return 0;
}
