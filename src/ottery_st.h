#ifndef OTTERY_ST_H_HEADER_INCLUDED_
#define OTTERY_ST_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

struct ottery_config;

#ifndef OTTERY_INTERNAL
struct __attribute__((aligned(16))) ottery_state {
  uint8_t dummy_[1024];
};
#endif

/* Functions that take or manipulate state explicitly. */
int ottery_st_init(struct ottery_state *st, const struct ottery_config *cfg);
void ottery_st_add_seed(struct ottery_state *st, const uint8_t *seed, size_t n);
void ottery_st_wipe(struct ottery_state *st);
void ottery_st_flush(struct ottery_state *st);
void ottery_st_stir(struct ottery_state *st);

void ottery_st_rand_bytes(struct ottery_state *st, void *out, size_t n);
unsigned ottery_st_rand_unsigned(struct ottery_state *st);
uint64_t ottery_st_rand_uint64(struct ottery_state *st);
unsigned ottery_st_rand_range(struct ottery_state *st, unsigned top);
uint64_t ottery_st_rand_range64(struct ottery_state *st, uint64_t top);

#endif
