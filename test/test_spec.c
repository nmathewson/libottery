#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define OTTERY_INTERNAL
#include "ottery.h"
#include "ottery_st.h"
#include "ottery-internal.h"

struct ottery_state state;

void
dump_hex(uint8_t *bytes, size_t n)
{
  printf("\"");
  while (n--) {
    uint8_t b = *bytes++;
    printf("%02x", (unsigned)b);
  }
  puts("\"");
}
unsigned
hexnybble(char c)
{
  switch (c) {
  case '0': return 0;
  case '1': return 1;
  case '2': return 2;
  case '3': return 3;
  case '4': return 4;
  case '5': return 5;
  case '6': return 6;
  case '7': return 7;
  case '8': return 8;
  case '9': return 9;
  case 'a': case 'A': return 10;
  case 'b': case 'B': return 11;
  case 'c': case 'C': return 12;
  case 'd': case 'D': return 13;
  case 'e': case 'E': return 14;
  case 'f': case 'F': return 15;
  default: abort();
  }

}
int
get_hex(const char *hex_, uint8_t *out)
{
  const char *hex = hex_;
  while (*hex) {
    if (!hex[1]) { printf("odd-length hex string %s\n", hex_); return 1; }
    if (! (isxdigit(hex[0]) && isxdigit(hex[1]))) {
      printf("bad digit in hex string %s\n", hex_); return 2;
    }
    *out++ = (hexnybble(hex[0]) << 4) | hexnybble(hex[1]);
    hex += 2;
  }
  return 0;
}

#define u32                                     \
  do {                                          \
    uint32_t u = ottery_st_rand_uint32(&state); \
    printf("%u\n", (unsigned) u);               \
  } while (0);
#define u64                                                 \
  do {                                                      \
    uint64_t u = ottery_st_rand_uint64(&state);             \
    printf("%llu\n", (unsigned long long) u);               \
  } while (0);
#define stir                                    \
  do {                                          \
    ottery_st_stir(&state);                     \
  } while (0);
#define nBytes(num)                             \
  do {                                          \
    size_t n = (num);                           \
    uint8_t *x = malloc(n);                     \
    if (!x) { perror("malloc"); exit(1); }      \
    ottery_st_rand_bytes(&state, x, n);         \
    dump_hex(x,n);                              \
    free(x);                                    \
  } while (0);
#define addHexSeed(seed)                        \
  do {                                          \
    const char *s = (seed);                     \
    uint8_t *x = malloc(strlen(s)/2 + 1);       \
    if (!x) { perror("malloc"); exit(1); }      \
    if (get_hex(s,x)) { exit(1); }              \
    ottery_st_add_seed(&state, x, strlen(s)/2); \
  } while (0);

#define repeatM(n)                              \
  for (i = 0; i < (n); ++i)

void
demo(void)
{
  /* This ugliness is a kludge so that the C and the Haskell can have the same
   * code here.
   */

	u32
	u32
	stir
	u32
	addHexSeed ("f00d")
	nBytes (256)
	u64
}

int
main(int argc, char **argv)
{
  (void) argc;
  (void) argv;

  if (argc < 2) {
    puts("I need the location of test_spec_seed"); return 1;
  }

  struct ottery_config config;
  ottery_config_init(&config);
  ottery_config_set_urandom_device_(&config, argv[1]);
  ottery_config_force_implementation(&config, OTTERY_PRF_CHACHA8);

  if (ottery_st_init(&state, &config)) {
    printf("couldn't initialize state\n"); return 1;
  }
  demo();

  return 0;
}
