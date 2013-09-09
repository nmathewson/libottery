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
/**
 * @file test_egd.c
 *
 * Tests for the EGD entropy backend.
 */
#include "ottery-internal.h"
#include "ottery.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
  struct sockaddr_un sun;
  struct ottery_osrng_config cfg;
  unsigned char buf[257];
  unsigned char scratch[257];
  long n;
  char *endp;
  uint32_t flags;
  int result;

  /* Enable test over inet */
  if (argc < 3) {
    printf("I need a number of bytes and the address of an egd socket\n");
    return 1;
  }
  n = strtol(argv[1], &endp, 10);
  if (n < 0 || n > 256 || *endp) {
    printf("First argument must be in 0..256\n");
    return 1;
  }
  if (strlen(argv[2])+1 >= sizeof(sun.sun_path)) {
    printf("Path is too long\n");
    return 1;
  }

  memset(&sun, 0, sizeof(sun));
  memset(&cfg, 0, sizeof(cfg));

  sun.sun_family = AF_UNIX;
  if (!strcmp(argv[2], "_BROKEN_FAMILY_"))
    sun.sun_family = 0xf0;

  memcpy(sun.sun_path, argv[2], strlen(argv[2])+1);
  cfg.egd_sockaddr = (void*)&sun;
  cfg.egd_socklen = sizeof(sun);
  cfg.disabled_sources =
    OTTERY_ENTROPY_ALL_SOURCES & ~OTTERY_ENTROPY_SRC_EGD;

  result = ottery_os_randbytes_(&cfg, 0, buf, (size_t) n, scratch, &flags);

  if (result == 0) {
    int i;
    printf("FLAGS:%x\n",flags);
    printf("BYTES:");
    for (i=0; i<n; ++i) printf("%02x", buf[i]);
    puts("");
  } else {
    printf("ERR:%d\n",result);
  }
  return 0;
}


