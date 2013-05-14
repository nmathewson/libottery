#ifndef OTTERY_H_HEADER_INCLUDED_
#define OTTERY_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

#ifndef OTTERY_INTERNAL
struct __attribute__((aligned(16))) ottery_state {
  uint8_t dummy_[1024];
};
struct ottery_config {
  uint8_t dummy_[1024];
};
#else
struct ottery_state;
struct ottery_config;
#endif

/* Error codes */
#define OTTERY_ERR_NONE                  0x0000
#define OTTERY_ERR_LOCK_INIT             0x0001
#define OTTERY_ERR_INTERNAL              0x0002
#define OTTERY_ERR_INIT_STRONG_RNG       0x0003
#define OTTERY_ERR_ACCESS_STRONG_RNG     0x0004
#define OTTERY_ERR_INVALID_ARGUMENT      0x0005
#define OTTERY_ERR_STATE_ALIGNMENT       0x0006

#define OTTERY_ERR_STATE_INIT            0x1000
#define OTTERY_ERR_FLAG_GLOBAL_PRNG_INIT 0x2000
#define OTTERY_ERR_FLAG_POSTFORK_RESEED  0x4000

/* Functions to interact with the library on a global level */
void ottery_set_fatal_handler(void (*fn)(int errorcode));

/* Functions to manipulate parameters. */
#define OTTERY_CHACHA   "CHACHA"
#define OTTERY_CHACHA8  "CHACHA8"
#define OTTERY_CHACHA12 "CHACHA12"
#define OTTERY_CHACHA20 "CHACHA20"
#define OTTERY_CHACHA_SIMD   "CHACHA-SIMD"
#define OTTERY_CHACHA8_SIMD  "CHACHA8-SIMD"
#define OTTERY_CHACHA12_SIMD "CHACHA12-SIMD"
#define OTTERY_CHACHA20_SIMD "CHACHA20-SIMD"
#define OTTERY_CHACHA_NO_SIMD   "CHACHA-NOSIMD"
#define OTTERY_CHACHA8_NO_SIMD  "CHACHA8-NOSIMD"
#define OTTERY_CHACHA12_NO_SIMD "CHACHA12-NOSIMD"
#define OTTERY_CHACHA20_NO_SIMD "CHACHA20-NOSIMD"

int ottery_config_init(struct ottery_config *cfg);
int ottery_config_set_option(struct ottery_config *cfg, uint32_t option);
int ottery_config_force_implementation(struct ottery_config *cfg,
                                       const char *impl);

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


/* Functions that use an implicit global state */
int ottery_init(const struct ottery_config *cfg);
void ottery_add_seed(const uint8_t *seed, size_t n);
void ottery_wipe(void);
void ottery_stir(void);

void ottery_rand_bytes(void *out, size_t n);
unsigned ottery_rand_unsigned(void);
uint64_t ottery_rand_uint64(void);
unsigned ottery_rand_range(unsigned top);
uint64_t ottery_rand_range64(uint64_t top);


#endif
