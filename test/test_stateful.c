#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ottery.h"
#include "ottery-internal.h"

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
    return ottery_stream_chacha8_(out, outlen, st);
  case 20:
    return ottery_stream_chacha20_(out, outlen, st);
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

static unsigned
stream_chunked(const struct ottery_prf *prf, void *prf_state,
               uint8_t *output, uint32_t outlen, uint32_t idx)
{
  unsigned written=0;
  while (outlen >= prf->output_len) {
    prf->generate(prf_state, output, idx);
    idx += prf->idx_step;
    output += prf->output_len;
    outlen -= prf->output_len;
    written += prf->output_len;
  }
  return written;
}

static void
check_stateful(int rounds, uint64_t initial_counter, int check_chunked)
{
  unsigned char *buf1;
  unsigned char *buf2;
  struct chacha_state st;
  unsigned step;
  const struct ottery_prf *prf = NULL;
  uint8_t ost[MAX_STATE_LEN];
#define LEN 16384
  if (rounds == 8)
    prf = &ottery_prf_chacha8_;
  else if (rounds == 20)
    prf = &ottery_prf_chacha20_;
  else
    abort();
  buf1 = malloc(LEN);
  buf2 = malloc(LEN);
  ottery_chacha_state_setup_(&st,
                     (uint8_t*)"Come in, Ftumch! Your time is up",
                     (uint8_t*)"nmklpjkl",
                     initial_counter);
  stream(rounds, buf1, LEN, &st);
  for (step = 1; step <= 17; ++step) {
    memset(buf2, 0, LEN);
    ottery_chacha_state_setup_(&st,
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
  if (! check_chunked)
    goto done;
  prf->setup(&ost, (uint8_t*)"Come in, Ftumch! Your time is upnmklpjkl");
  unsigned n = stream_chunked(prf, ost, buf2, LEN, (uint32_t)initial_counter);
  if (n < prf->output_len*3) {
    printf("nothing or little generated (%d, %d)\n", n, prf->output_len);
    exit(1);
  }
  if (memcmp(buf1, buf2, n)) {
    printf("Error with chunked rng\n");
    first_diff(buf1, buf2, LEN);
    exit(1);
  }

 done:
  free(buf1);
  free(buf2);
}

int
main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
  puts("8");
  check_stateful(8, 0, 1);
  puts("20");
  check_stateful(20, 0, 1);

  puts("8+");
  check_stateful(8, 12345, 1);
  puts("20+");
  check_stateful(20, 12345, 1);

  puts("8++");
  check_stateful(8, 0xffffffffeULL, 0);
  puts("20++");
  check_stateful(20, 0xfffffffeULL, 0);

  puts("OK");
  return 0;
}
