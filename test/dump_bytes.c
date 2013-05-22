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
#include "ottery_st.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ottery_state state;

int use_threads = 0;
int use_global_state = 0;

#define STATE() (&state)
#define USING_STATE() (use_global_state)
#include "st_wrappers.h"

#define BUF_LEN 8192

static int generator_param = 0;
static int (*generate)(uint8_t *buf) = NULL;

static int gen_rand_bytes(uint8_t *buf);
static int gen_randnum_bytes(uint8_t *buf);
static int gen_u32(uint8_t *buf);
static int gen_u64(uint8_t *buf);
static int gen_u32_odd(uint8_t *buf);
static int gen_u64_odd(uint8_t *buf);
static int chop_range8(uint8_t *buf);
static int chop_range16(uint8_t *buf);

static const struct {
  const char *name;
  int takes_argument;
  int (*fn)(uint8_t*);
} strategies[] = {
  { "rand_bytes", 1, gen_rand_bytes },
  { "randnum_bytes", 0, gen_randnum_bytes },
  { "u32", 0, gen_u32 },
  { "u64", 0, gen_u64 },
  { "u32_odd", 0, gen_u32_odd },
  { "u64_odd", 0, gen_u64_odd },
  { "chop_range8", 1, chop_range8 },
  { "chop_range16", 1, chop_range16 },
  { NULL, 0, NULL },
};

static void
usage(void)
{
  int i;
  puts("dump_bytes [-i implementation] [-s strategy] [-T]");
  puts("   implementations include CHACHA{8,12,20}{,-SIMD,-NOSIMD}");
  puts("   strategies are:");
  for (i=0; strategies[i].name; ++i) {
    printf("      %s%s\n", strategies[i].name,
           strategies[i].takes_argument ? ":<N>" : "");
  }
  puts("   recommended usage is:");
  puts("      dump_bytes [options] | dieharder -g 200 -a ");
}

static int
parse_strategy(const char *s)
{
  size_t len = strlen(s);
  const char *colon;
  if (NULL != (colon = strchr(s, ':'))) {
    len = colon - s;
  }

  int i;
  for (i=0; strategies[i].name; ++i) {
    if (!strncmp(strategies[i].name, s, len) && strlen(s) == len) {
      if (!colon != !strategies[i].takes_argument) {
        fprintf(stderr, "missing or unexpected argument for %s\n",
                strategies[i].name);
        return -1;
      }
      if (colon)
        generator_param = atoi(colon+1);
      generate = strategies[i].fn;
      return 0;
    }
  }

  return -1;
}

static int
gen_rand_bytes(uint8_t *buf)
{
  if (generator_param == 0 || generator_param > BUF_LEN)
    return -1;
  OTTERY_RAND_BYTES(buf, generator_param);
  return generator_param;
}

static int
gen_randnum_bytes(uint8_t *buf)
{
  uint8_t x;
  do {
    OTTERY_RAND_BYTES(&x, 1);
  } while (! x);
  OTTERY_RAND_BYTES(buf, x);
  return x;
}

static int
gen_u32(uint8_t *buf)
{
  unsigned u = OTTERY_RAND_UNSIGNED();
  memcpy(buf, &u, sizeof(u));
  return sizeof(u);
}

static int
gen_u64(uint8_t *buf)
{
  uint64_t u = OTTERY_RAND_UINT64();
  memcpy(buf, &u, sizeof(u));
  return sizeof(u);
}

static int
gen_u32_odd(uint8_t *buf)
{
  unsigned u = OTTERY_RAND_UNSIGNED();
  uint8_t x;
  memcpy(buf, &u, sizeof(u));
  if ((u & 7) == 0)
    OTTERY_RAND_BYTES(&x, 1);
  return sizeof(u);
}

static int
gen_u64_odd(uint8_t *buf)
{
  uint64_t u = OTTERY_RAND_UINT64();
  uint8_t x;
  memcpy(buf, &u, sizeof(u));
  if ((u & 7) == 0)
    OTTERY_RAND_BYTES(&x, 1);
  return sizeof(u);
}

static int
chop_range8(uint8_t *buf)
{
  if (generator_param < 255)
    return -1;
  unsigned u;
  do {
    u = OTTERY_RAND_RANGE(generator_param);
  } while (u > 255);
  *buf = (uint8_t)u;
  return 1;
}

static int
chop_range16(uint8_t *buf)
{
  if (generator_param < 65535)
    return -1;
  unsigned u;
  uint16_t u16;
  do {
    u = OTTERY_RAND_RANGE(generator_param);
  } while (u > 65535);
  u16 = (uint16_t)u;
  memcpy(buf, &u16, 2);
  return 2;
}

int
main(int argc, char **argv)
{
  int ch;
  struct ottery_config cfg;

  if (ottery_config_init(&cfg)) {
    fprintf(stderr, "Couldn't initialize ottery_config");
    return 1;
  }

  while ((ch = getopt(argc, argv, "i:s:TGh?")) != -1) {
    switch (ch) {
    case '?': case 'h':
      usage();
      return 0;
    case 'i':
      if (ottery_config_force_implementation(&cfg, optarg) < 0) {
        fprintf(stderr, "Unsupported/unrecognized PRF \"%s\"\n", optarg);
        return 1;
      }
      break;
    case 's':
      if (parse_strategy(optarg) < 0) {
        fprintf(stderr, "Unrecognized/unsupported strategy \"%s\"\n", optarg);
        return 1;
      }
      break;
    case 'T':
      use_threads = 1;
      { fprintf(stderr, "-T not yet supported\n"); return 1; }
      break;
    case 'G':
      use_global_state = 1;
      break;
    default:
      usage();
      return 1;
    }
  }

  if (use_global_state ? ottery_init(&cfg) : ottery_st_init(&state, &cfg)) {
    fprintf(stderr, "Libottery initialization failed.\n");
    return 1;
  }

  if (!generate) {
    generate = gen_rand_bytes;
    generator_param = BUF_LEN;
  }

  while (1) {
    uint8_t buf[BUF_LEN];
    int n = generate(buf);
    if (n < 0) {
      fprintf(stderr, "Invalid parameter\n");
      return 1;
    }
    write(1, buf, n);
  }

  return 0;
}
