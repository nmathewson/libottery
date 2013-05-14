#ifndef OTTERY_H_HEADER_INCLUDED_
#define OTTERY_H_HEADER_INCLUDED_
#include <stdint.h>
#include <sys/types.h>

struct ottery_config;
struct ottery_state;

/* Functions that use an implicit global state */
void ottery_rand_bytes(void *out, size_t n);
unsigned ottery_rand_unsigned(void);
uint64_t ottery_rand_uint64(void);
unsigned ottery_rand_range(unsigned top);
uint64_t ottery_rand_range64(uint64_t top);

int ottery_init(const struct ottery_config *cfg);
void ottery_add_seed(const uint8_t *seed, size_t n);
void ottery_wipe(void);
void ottery_stir(void);

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
int ottery_config_force_implementation(struct ottery_config *cfg,
                                       const char *impl);

#ifndef OTTERY_INTERNAL
struct ottery_config {
  uint8_t dummy_[1024];
};
#endif

#endif
