#ifndef OTTERY_H_HEADER_INCLUDED_
#define OTTERY_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

#ifndef OTTERY_INTERNAL
struct ottery_state {
  uint8_t dummy_[1024];
};
struct ottery_config {
  uint8_t dummy_[1024];
};
#else
struct ottery_state;
struct ottery_config;
#endif

/* Functions to manipulate parameters. */
#define OTTERY_CHACHA   "CHACHA"
#define OTTERY_CHACHA8  "CHACHA8"
#define OTTERY_CHACHA12 "CHACHA12"
#define OTTERY_CHACHA20 "CHACHA20"

int ottery_config_init(struct ottery_config *cfg);
int ottery_config_force_implementation(struct ottery_config *cfg,
                                       const char *impl);

/* Functions that take or manipulate state explicitly. */
int ottery_st_init(struct ottery_state *st, const struct ottery_config *cfg);
void ottery_st_add_seed(struct ottery_state *st, uint8_t *seed, size_t n);
void ottery_st_wipe(struct ottery_state *st);
void ottery_st_flush(struct ottery_state *st);
void ottery_st_stir(struct ottery_state *st);

void ottery_st_rand_bytes(struct ottery_state *st, void *out, size_t n);
unsigned ottery_st_rand_unsigned(struct ottery_state *st);
uint64_t ottery_st_rand_uint64(struct ottery_state *st);
unsigned ottery_st_rand_range(struct ottery_state *st, unsigned top);
uint64_t ottery_st_rand_range64(struct ottery_state *st, uint64_t top);

#endif
