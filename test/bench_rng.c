#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "chacha.h"

struct chacharand_state {
  uint8_t junk[1024];
};

struct chacharand_state s8;
struct chacharand_state s20;

int chacharand8_init(struct chacharand_state *st);
int chacharand20_init(struct chacharand_state *st);
unsigned chacharand8_unsigned(struct chacharand_state *st);
unsigned chacharand20_unsigned(struct chacharand_state *st);
void chacharand8_bytes(struct chacharand_state *st, void *bytes, size_t n);
void chacharand20_bytes(struct chacharand_state *st, void *bytes, size_t n);

#define N 10000000

#define TIME_UNSIGNED_RNG(rng_fn) do {          \
    struct timeval start, end;                  \
    unsigned accumulator = 0;                   \
    int i;                                      \
    gettimeofday(&start, NULL);                 \
    for (i=0; i < N; ++i) {                     \
      accumulator += rng_fn;                    \
    }                                           \
    gettimeofday(&end, NULL);                   \
    uint64_t usec = end.tv_sec - start.tv_sec;  \
    usec *= 1000000;\
    usec += end.tv_usec - start.tv_usec;        \
    printf("%s: %f nsec per call\n", __func__, (usec*1000.0)/N); \
  } while (0)

void
time_chacharand8(void)
{
  TIME_UNSIGNED_RNG( (chacharand8_unsigned(&s8)) );
}

void
time_chacharand20(void)
{
  TIME_UNSIGNED_RNG( (chacharand20_unsigned(&s20)) );
}

void
time_arc4random(void)
{
  TIME_UNSIGNED_RNG( (arc4random()) );
}

void
time_libc_random(void)
{
  TIME_UNSIGNED_RNG( (random()) );
}


int main(int argc, char **argv)
{
  chacharand8_init(&s8);
  chacharand20_init(&s20);

  time_chacharand8();
  time_chacharand20();
  time_arc4random();
  time_libc_random();

  return 0;
}

