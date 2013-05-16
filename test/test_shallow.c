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
/**
 * @file test_shallow.c
 *
 * "Shallow" unit tests that try to make sure of various black-box and
 * glass-box properties of the libottery code without investigating how it
 * calls its backends.
 */
#define OTTERY_INTERNAL
#include "ottery.h"
#include "ottery_st.h"
#include "ottery-internal.h"

#include "tinytest.h"
#include "tinytest_macros.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char *state_allocation = NULL;
struct ottery_state *state = NULL;

#define OT_ENABLE_STATE TT_FIRST_USER_FLAG

void *
setup_state(const struct testcase_t *testcase)
{
  if (state)
    return NULL;
  if (testcase->flags & OT_ENABLE_STATE) {
    state_allocation = malloc(ottery_get_sizeof_state() + 16);
    const int misalign = (int) (((uintptr_t)state_allocation) & 0xf);
    state = (struct ottery_state *)( state_allocation + ((16-misalign)&0xf) );
    ottery_st_init(state, NULL);
  }
  return (void*) 1;
}
int
cleanup_state(const struct testcase_t *testcase, void *ptr)
{
  int retval = 1;
  (void)testcase;
  (void)ptr;
  if (state) {
    if (state->block_counter > 512 ||
        state->pos >= state->prf.output_len ||
        state->pid != getpid()) {
      retval = 0;
    }
  }
  if (state_allocation) {
    free(state_allocation);
  }
  return retval;
}
struct testcase_setup_t setup = {
  setup_state, cleanup_state
};

#define OTTERY_RAND_BYTES(out, n)               \
  (state ?                                      \
   ottery_st_rand_bytes(state, (out), (n)) :    \
   ottery_rand_bytes((out),(n)))

#define OTTERY_RAND_UNSIGNED()                  \
  (state ? ottery_st_rand_unsigned(state) : ottery_rand_unsigned())

#define OTTERY_RAND_UINT64()                                    \
  (state ? ottery_st_rand_uint64(state) : ottery_rand_uint64())

#define OTTERY_RAND_RANGE(n)                                    \
  (state ? ottery_st_rand_range(state, (n)) : ottery_rand_range(n))

#define OTTERY_RAND_RANGE64(n)                  \
  (state ? ottery_st_rand_range64(state, (n)) : ottery_rand_range64(n))

#define OTTERY_WIPE()                           \
  (state ? ottery_st_wipe(state) : ottery_wipe())

#define OTTERY_STIR()                                           \
  (state ? ottery_st_stir(state) : ottery_stir())

#define OTTERY_ADD_SEED(seed,n)                                  \
  (state ? ottery_st_add_seed(state,(seed),(n)) : ottery_add_seed((seed),(n)))

#define OTTERY_INIT(cfg)                                        \
  (state ? ottery_st_init(state,(cfg)) : ottery_init(cfg))

static void
test_get_sizeof(void *arg)
{
  (void)arg;
  tt_int_op(ottery_get_sizeof_config(), <=, sizeof(struct ottery_config));
  tt_int_op(ottery_get_sizeof_state(), <=, sizeof(struct ottery_state));
 end:
  ;
}

static void
test_osrandom(void *arg)
{
  (void) arg;
  uint8_t buf[65];
  int i, j;

  memset(buf, 0, sizeof(buf));
  /* randbytes must succeed. */
  tt_int_op(0, ==, ottery_os_randbytes_(NULL, buf, 63));

  /* Does it stop after the number of bytes we tell it? */
  tt_int_op(0, ==, buf[63]);
  tt_int_op(0, ==, buf[64]);

  /* Try it 8 times; make sure every byte is set at least once */
  for (i = 0; i < 7; ++i) {
    uint8_t buf2[63];
    tt_int_op(0, ==, ottery_os_randbytes_(NULL, buf2, 63));
    for (j = 0; j < 63; ++j)
      buf[j] |= buf2[j];
  }
  for (j = 0; j < 63; ++j)
    tt_int_op(0, !=, buf[j]);

  tt_int_op(OTTERY_ERR_INIT_STRONG_RNG, ==,
            ottery_os_randbytes_("/dev/please-dont-add-this-device", buf, 10));

  tt_int_op(OTTERY_ERR_ACCESS_STRONG_RNG, ==,
            ottery_os_randbytes_("/dev/null", buf, 10));

 end:
  ;
}

static void
test_single_buf(size_t n)
{
  unsigned offset, i, j;
  uint8_t *b = malloc(n + 32);
  uint8_t *sum = malloc(n);

  tt_assert(b);
  tt_assert(sum);

  for (offset = 0; offset < 16; ++offset) {
    memset(b, 0, n+32);
    memset(sum, 0, n);
    for (i=0; i<100; ++i) {
      OTTERY_RAND_BYTES(b + offset, n);

      /* Make sure that it doesn't write outside of its range. */
      for (j=0; j<offset; ++j) {
        tt_int_op(b[j], ==, 0);
      }
      for (j=n+offset; j<n+32; ++j) {
        tt_int_op(b[j], ==, 0);
      }

      /* Make sure every in-range byte sometimes gets written to. */
      for (j=0; j<n; ++j) {
        sum[j] |= b[offset+j];
      }
    }
    for (j=0; j<n; ++j) {
      tt_int_op(sum[j], ==, 0xff);
    }
  }

 end:
  if (sum)
    free(sum);
  if (b)
    free(b);
}

static void
test_rand_little_buf(void *arg)
{
  (void)arg;
  test_single_buf(1);
  test_single_buf(4);
  test_single_buf(7);
  test_single_buf(30);
  test_single_buf(63);
}

static void
test_rand_big_buf(void *arg)
{
  (void)arg;
  test_single_buf(64);
  test_single_buf(79);
  test_single_buf(80);
  test_single_buf(120);
  test_single_buf(1024);
}

static void
test_rand_uint(void *arg)
{
  (void)arg;
  int i;
  unsigned acc = 0, dec=(unsigned)-1;
  uint64_t acc64 = 0, dec64=(uint64_t)-1;
  uint8_t buf;
  /* Not much we can do to black-box test rand_unsinged and rand_uint64 other
     than to make sure that they don't crash and that the values are
     random-ish. For real results, we'll need the deep tests, plus statistical
     examining of dump_bytes.c output.
  */

  for (i = 0; i < 100; ++i) {
    unsigned u = OTTERY_RAND_UNSIGNED();
    uint64_t u64 = OTTERY_RAND_UINT64();
    acc |= u;
    acc64 |= u64;
    dec &= u;
    dec64 &= u64;
  }

  tt_int_op(acc, ==, (unsigned)-1);
  tt_assert(acc64 == (uint64_t)-1);
  tt_int_op(dec, ==, 0);
  tt_assert(dec64 == 0);

  acc = 0;
  acc64 = 0;
  dec = (unsigned)-1;
  dec64 = (uint64_t)-1;
  for (i = 0; i < 100; ++i) {
    unsigned u = OTTERY_RAND_UNSIGNED();
    uint64_t u64 = OTTERY_RAND_UINT64();
    OTTERY_RAND_BYTES(&buf, 1); /* make sure unaligned works. */
    acc |= u;
    acc64 |= u64;
    dec &= u;
    dec64 &= u64;

  }
  tt_int_op(acc, ==, (unsigned)-1);
  tt_assert(acc64 == (uint64_t)-1);
  tt_int_op(dec, ==, 0);
  tt_assert(dec64 == 0);

 end:
  ;
}

static void
test_range(void *arg)
{
  int i;
  int count[5];
  const uint64_t quite_big = ((uint64_t)1)<<60;
  int got_a_big_small_one = 0;
  int got_a_big_one = 0;
  (void)arg;

  for (i=0;i<1000;++i) {
    tt_int_op(OTTERY_RAND_RANGE(5), <=, 5);
    count[OTTERY_RAND_RANGE(5)] += 1;
    tt_int_op(OTTERY_RAND_RANGE(10), <=, 10);
    tt_int_op(OTTERY_RAND_RANGE(100), <=, 100);
    tt_int_op(OTTERY_RAND_RANGE64(100), <=, 100);
    tt_int_op(OTTERY_RAND_RANGE64(10000), <=, 10000);
    tt_int_op(OTTERY_RAND_RANGE64(quite_big), <=, quite_big);
    if (OTTERY_RAND_RANGE64(quite_big) > (((uint64_t)1)<<40))
      ++got_a_big_one;
    if (OTTERY_RAND_RANGE(3000000000U) > 2000000000U)
      ++got_a_big_small_one;
  }
  for (i=0; i<5; ++i) {
    tt_int_op(0, !=, count[i]);
  }
  tt_int_op(got_a_big_one, !=, 0);
  tt_int_op(got_a_big_small_one, !=, 0);

 end:
  ;
}

void
test_fork(void *arg)
{
  (void) arg;
#ifdef _WIN32
  tt_skip(); 
 end:
  ;
#else
  uint8_t buf[256];
  uint8_t buf2[256];
  int fd[2] = {-1, -1};
  pid_t p;

  if (pipe(fd) < 0)
    tt_abort_perror("pipe");

  if (!state)
    ottery_init(NULL);

  if ((p = fork()) == 0) {
    /* child */
    OTTERY_RAND_BYTES(buf, sizeof(buf));
    if (write(fd[1], buf, sizeof(buf)) < 0) {
      perror("write");
    }
    exit(0);
  } else if (p == -1) {
    tt_abort_perror("fork");
  }
  /* parent. */
  OTTERY_RAND_BYTES(buf, sizeof(buf));
  tt_int_op(sizeof(buf2), ==, read(fd[0], buf2, sizeof(buf2)));
  tt_assert(memcmp(buf, buf2, sizeof(buf)))
  tt_assert(memcmp(buf, buf2, 16))

 end:
  if (fd[0] >= 0)
    close(fd[0]);
  if (fd[1] >= 0)
    close(fd[1]);
#endif
}

void
test_bad_init(void *arg)
{
  (void)arg;

  struct ottery_config cfg;
  struct ottery_prf bad_prf;

  tt_int_op(0, ==, ottery_config_init(&cfg));
  tt_int_op(OTTERY_ERR_INVALID_ARGUMENT, ==,
            ottery_config_force_implementation(&cfg, 0));
  tt_int_op(OTTERY_ERR_INVALID_ARGUMENT, ==,
            ottery_config_force_implementation(&cfg, "rc4"));
  tt_int_op(0, ==,
            ottery_config_force_implementation(&cfg, "CHACHA"));

  memcpy(&bad_prf, &ottery_prf_chacha20_, sizeof(struct ottery_prf));
  bad_prf.state_len = 1024;
  ottery_config_set_manual_prf_(&cfg, &bad_prf);
  tt_int_op(OTTERY_ERR_INTERNAL, ==, OTTERY_INIT(&cfg));

  memcpy(&bad_prf, &ottery_prf_chacha20_, sizeof(struct ottery_prf));
  bad_prf.state_bytes = 2000;
  ottery_config_set_manual_prf_(&cfg, &bad_prf);
  tt_int_op(OTTERY_ERR_INTERNAL, ==, OTTERY_INIT(&cfg));

  memcpy(&bad_prf, &ottery_prf_chacha20_, sizeof(struct ottery_prf));
  bad_prf.output_len = 8;
  ottery_config_set_manual_prf_(&cfg, &bad_prf);
  tt_int_op(OTTERY_ERR_INTERNAL, ==, OTTERY_INIT(&cfg));

  ottery_config_force_implementation(&cfg, "CHACHA");
  tt_int_op(0, ==, OTTERY_INIT(&cfg));

  ottery_config_set_urandom_device_(&cfg,"/dev/please-dont-add-this-device");
  tt_int_op(OTTERY_ERR_INIT_STRONG_RNG, ==, OTTERY_INIT(&cfg));

  ottery_config_set_urandom_device_(&cfg,"/dev/null");
  tt_int_op(OTTERY_ERR_ACCESS_STRONG_RNG, ==, OTTERY_INIT(&cfg));

  ottery_config_set_urandom_device_(&cfg, NULL);
  tt_int_op(0, ==, OTTERY_INIT(&cfg));

 end:
  ;
}

void
test_reseed_stir(void *arg)
{
  (void)arg;
  uint8_t buf[256], buf2[256];
  struct ottery_state st_orig;
  int op;

  OTTERY_RAND_UNSIGNED();
  if (state) {
    memcpy(&st_orig, state, sizeof(st_orig));
  }
  OTTERY_RAND_BYTES(buf, sizeof(buf));
  for (op = 0; op < 6; ++op) {
    if (state)
      memcpy(state, &st_orig, sizeof(st_orig));
    if (op == 0) {
      ;
    } else if (op == 1) {
      OTTERY_STIR();
    } else if (op == 2) {
      OTTERY_ADD_SEED((const uint8_t*)"chosen by fair dice roll. "
                      "guaranteed to be random.",50);
    } else if (op == 3) {
      OTTERY_ADD_SEED(NULL, 6060842);
    } else if (op == 4) {
      OTTERY_ADD_SEED((const uint8_t*)"", 0);
    } else if (op == 5) {
      OTTERY_WIPE();
      if (state)
        OTTERY_INIT(NULL);
    }
    OTTERY_RAND_BYTES(buf2, sizeof(buf2));
    if (op == 0 && state) {
      tt_assert(0 == memcmp(buf, buf2, sizeof(buf)));
    } else {
      tt_assert(memcmp(buf, buf2, sizeof(buf)));
      tt_assert(memcmp(buf, buf2, 16));
    }
  }

 end:
  ;
}

static void
test_misaligned_init(void *arg)
{
  (void)arg;
  char *ptr;
  struct ottery_state *st;

  ptr = malloc(sizeof(struct ottery_state) + 1);
  tt_assert(ptr);

  if ((((uintptr_t)ptr) & 0xf) == 0) {
    st = (void*)(ptr+1);
  } else {
    st = (void*)ptr;
  }
  /* Now st is misaligned. */

  tt_int_op(OTTERY_ERR_STATE_ALIGNMENT, ==, ottery_st_init(st, NULL));

 end:
  if (ptr)
    free(ptr);
}

static int got_fatal_err = 0;
static void
fatal_handler(int err)
{
  if (got_fatal_err != 0 || err == 0)
    got_fatal_err = -1;
  else
    got_fatal_err = err;
}

static void
test_fatal(void *arg)
{
  (void)arg;
  __attribute__((aligned(16))) struct ottery_state st;
  uint8_t buf[8];

  ottery_set_fatal_handler(fatal_handler);
  memset(&st, 0xff, sizeof(st));

  /* Fatal errors from */
  got_fatal_err = 0;
  ottery_st_add_seed(&st, (const uint8_t*)"xyz", 3);
  tt_int_op(got_fatal_err, ==, OTTERY_ERR_STATE_INIT);

  got_fatal_err = 0;
  ottery_st_rand_unsigned(&st);
  tt_int_op(got_fatal_err, ==, OTTERY_ERR_STATE_INIT);

  got_fatal_err = 0;
  ottery_st_rand_bytes(&st, buf, 8);
  tt_int_op(got_fatal_err, ==, OTTERY_ERR_STATE_INIT);

  /* Okay, now we're going to fake out the initialization-failure-postfork
   * code. */
  got_fatal_err = 0;
  tt_int_op(0, ==, ottery_st_init(&st, NULL));
  ottery_st_rand_unsigned(&st);
  st.pid = getpid() + 100; /* force a postfork reseed. */
  st.urandom_fname = "/dev/null"; /* make that reseed impossible */
  tt_int_op(got_fatal_err, ==, 0);
  ottery_st_rand_unsigned(&st);
  tt_int_op(got_fatal_err, ==,
            OTTERY_ERR_ACCESS_STRONG_RNG|OTTERY_ERR_FLAG_POSTFORK_RESEED);

 end:
  ;
}

struct testcase_t misc_tests[] = {
  { "osrandom", test_osrandom, 0, NULL, NULL },
  { "get_sizeof", test_get_sizeof, 0, NULL, NULL },
  { "fatal", test_fatal, TT_FORK, NULL, NULL },
  END_OF_TESTCASES
};

#define COMMON_TESTS(flags)                                            \
  { "range", test_range, TT_FORK|flags, &setup, NULL },                \
  { "unsigned", test_rand_uint, TT_FORK|flags, &setup, NULL },         \
  { "little_buf", test_rand_little_buf, TT_FORK|flags, &setup, NULL }, \
  { "big_buf", test_rand_big_buf, TT_FORK|flags, &setup, NULL },       \
  { "fork", test_fork, TT_FORK|flags, &setup, NULL },                  \
  { "bad_init", test_bad_init, TT_FORK|flags, &setup, NULL, },         \
  { "reseed_and_stir", test_reseed_stir, TT_FORK|flags, &setup, NULL, }

struct testcase_t stateful_tests[] = {
  COMMON_TESTS(OT_ENABLE_STATE),
  { "misaligned_init", test_misaligned_init, 0, NULL, NULL },
  END_OF_TESTCASES,
};

struct testcase_t global_tests[] = {
  COMMON_TESTS(0),
  END_OF_TESTCASES,
};

struct testgroup_t groups[] = {
  { "misc/", misc_tests },
  { "state/", stateful_tests },
  { "global/", global_tests },
  END_OF_GROUPS
};

int
main(int argc, const char **argv)
{
  setbuf(stdout, NULL);
  return tinytest_main(argc, argv, groups);
}
