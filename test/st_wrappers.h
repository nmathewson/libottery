
#define OTTERY_RAND_BYTES(out, n)                            \
  (USING_NOLOCK() ?                                          \
   ottery_st_rand_bytes_nolock(STATE_NOLOCK(), (out), (n)) : \
   USING_STATE() ?                                           \
   ottery_st_rand_bytes(STATE(), (out), (n)) :               \
   ottery_rand_bytes((out),(n)))

#define OTTERY_RAND_UNSIGNED()                                       \
  (USING_NOLOCK() ? ottery_st_rand_unsigned_nolock(STATE_NOLOCK()) : \
   USING_STATE() ? ottery_st_rand_unsigned(STATE()) : ottery_rand_unsigned())

#define OTTERY_RAND_UINT32()                                       \
  (USING_NOLOCK() ? ottery_st_rand_uint32_nolock(STATE_NOLOCK()) : \
   USING_STATE() ? ottery_st_rand_uint32(STATE()) : ottery_rand_uint32())

#define OTTERY_RAND_UINT64()                                       \
  (USING_NOLOCK() ? ottery_st_rand_uint64_nolock(STATE_NOLOCK()) : \
   USING_STATE() ? ottery_st_rand_uint64(STATE()) : ottery_rand_uint64())

#define OTTERY_RAND_RANGE(n)                                          \
  (USING_NOLOCK() ? ottery_st_rand_range_nolock(STATE_NOLOCK(),(n)) : \
   USING_STATE() ? ottery_st_rand_range(STATE(), (n)) : ottery_rand_range(n))

#define OTTERY_RAND_RANGE64(n)                                           \
  (USING_NOLOCK() ? ottery_st_rand_range64_nolock(STATE_NOLOCK(), (n)) : \
   USING_STATE() ? ottery_st_rand_range64(STATE(), (n)) :                \
   ottery_rand_range64(n))

#define OTTERY_WIPE()                                       \
  (USING_NOLOCK() ? ottery_st_wipe_nolock(STATE_NOLOCK()) : \
   USING_STATE() ? ottery_st_wipe(STATE()) : ottery_wipe())

#define OTTERY_STIR()                                       \
  (USING_NOLOCK() ? ottery_st_stir_nolock(STATE_NOLOCK()) : \
   USING_STATE() ? ottery_st_stir(STATE()) : ottery_stir())

#define OTTERY_ADD_SEED(seed,n)                                            \
  (USING_NOLOCK() ? ottery_st_add_seed_nolock(STATE_NOLOCK(),(seed),(n)) : \
   USING_STATE() ? ottery_st_add_seed(STATE(),(seed),(n)) :                \
   ottery_add_seed((seed),(n)))

#define OTTERY_INIT(cfg)                                          \
  (USING_NOLOCK() ? ottery_st_init_nolock(STATE_NOLOCK(),(cfg)) : \
   USING_STATE() ? ottery_st_init(STATE(),(cfg)) : ottery_init(cfg))
