#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chacha.h"

static void
first_diff(const uint8_t *aa, const uint8_t *bb, uint64_t len)
{
  uint64_t p;
  for (p = 0; p < len; ++p) {
    if (aa[p] != bb[p]) {
      printf("First difference at position %llu", (unsigned long long)p);
      return;
    }
  }
}

static int
stream(int rounds, uint8_t *out, uint64_t outlen, struct chacha_state *st)
{
  switch (rounds) {
  case 8:
    return crypto_stream_8(out, outlen, st);
  case 20:
    return crypto_stream_20(out, outlen, st);
  default:
    puts("ERROR");
    exit(1);
  }
}

static int
stream_stepped(int rounds, uint8_t *out, uint64_t outlen, unsigned step,
               struct chacha_state *st)
{
  uint64_t ctr;
  while (outlen > step*64) {
    ctr = st->block_counter;
    stream(rounds, out, step*64, st);
    out += step * 64;
    outlen -= step*64;
    if (st->block_counter != ctr + step) {
      printf("Step problem.\n");
      exit(1);
    }
  }
  if (outlen)
    stream(rounds, out, outlen, st);
  return 0;
}

static void
check_stateful(int rounds, uint64_t initial_counter)
{
  unsigned char *buf1;
  unsigned char *buf2;
  struct chacha_state st;
  unsigned step;
#define LEN 16384
  buf1 = malloc(LEN);
  buf2 = malloc(LEN);
  chacha_state_setup(&st,
                     (uint8_t*)"Come in, Ftumch! Your time is up",
                     (uint8_t*)"nmklpjkl",
                     initial_counter);
  stream(rounds, buf1, LEN, &st);
  for (step = 1; step <= 17; ++step) {
    memset(buf2, 0, LEN);
    chacha_state_setup(&st,
                       (uint8_t*)"Come in, Ftumch! Your time is up",
                       (uint8_t*)"nmklpjkl",
                       initial_counter);
    stream_stepped(rounds, buf2, LEN, step, &st);

    if (memcmp(buf1, buf2, LEN) != 0) {
      printf("Error at step==%d\n",step);
      first_diff(buf1, buf2, LEN);
      exit(1);
    }
  }
  free(buf1);
  free(buf2);
}

int
main(int argc, char **argv)
{
  puts("8");
  check_stateful(8, 0);
  puts("20");
  check_stateful(20, 0);

  puts("8+");
  check_stateful(8, 12345);
  puts("20+");
  check_stateful(20, 12345);

  puts("8++");
  check_stateful(8, 0xffffffffeULL);
  puts("20++");
  check_stateful(20, 0xfffffffeULL);

  puts("OK");
  return 0;
}
