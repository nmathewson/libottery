#include <unistd.h>
#include "chacha.h"

struct chacharand_state;
int chacharand8_init(struct chacharand_state *st);
void chacharand8_bytes(struct chacharand_state *st, void *bytes, size_t n);

struct chacharand_state {
  char dummy[1024];
} state;

int
main(int argc, char **argv)
{
  chacharand8_init(&state);
  while (1) {
    char buf[8192];
    chacharand8_bytes(&state, buf, 8192);
    write(1, buf, 8192);
  }

  return 0;
}
