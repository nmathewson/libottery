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
