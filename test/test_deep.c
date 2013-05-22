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
 * @file test_deepw.c
 *
 * "Deep" unit tests that try to make sure of how the ottery libcore invokes a
 * PRF backend.  We include a cheezy little fake PRF based on running a little
 * vigenere cipher on Lorem Ipsum, and then we make sure that the libottery
 * state evolves as we'd expect given its inputs.
 */
#define OTTERY_INTERNAL
#include "ottery.h"
#include "ottery_st.h"
#include "ottery-internal.h"

#include "tinytest.h"
#include "tinytest_macros.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define STATE() state
#define USING_STATE() (state != NULL)
#include "st_wrappers.h"

/* Here's the dummy generator we'll be using for tests.  It's based on
 * the classic 'lorem ipsum' text translation, and a 4-character
 * vigenere key. It is, obviously, not a good RNG. */

const char LOREM[] =
  "Nor again is there anyone who loves or pursues or desires to obtain pain "
  "of itself, because it is pain, but because occasionally circumstances "
  "occur in which toil and pain can procure him some great pleasure. To "
  "take a trivial example, which of us ever undertakes laborious physical "
  "exercise, except to obtain some advantage from it? But who has any right "
  "to find fault with a man who chooses to enjoy a pleasure that has no "
  "annoying consequences, or one who avoids a pain that produces no "
  "resultant pleasure?seb";
/* Translation of a part of Cicero's de Finibus Bonoum et Malorum
 * excerpted in the classic "Lorem ipsum" text, translated by H. Rackham
 * in 1914, quoted on lipsum.com. 512 characters long. */

char
rotate_char(char c1, int rotation)
{
  if (!isalpha(c1)) {
    uint8_t r = ' ' + rotation;
    if (r > (uint8_t)'9') r -= 26;
    //printf("%d %d -> %d\n", c1, rotation, r);
    return r;
  } else if (isupper(c1)) {
    uint8_t r = c1 + rotation;
    if (r > (uint8_t)'Z') r -= 26;
    //printf("%d %d -> %d\n", c1, rotation, r);
    return r;
  } else {
    uint8_t r = c1 + rotation;
    if (r > (uint8_t)'z') r -= 26;
    //printf("%d %d -> %d\n", c1, rotation, r);
    return r;
  }
}

struct dummy_prf_state {
  int rotation[4];
};

static void
dummy_prf_setup(void *state_, const uint8_t *bytes)
{
  struct dummy_prf_state *state = state_;
  int i;
  for (i = 0; i < 4; ++i) {
    int r;
    char ch = (char)bytes[i];
    if (islower(ch)) {
      r = ch - 'a';
    } else if (isupper(ch)) {
      r = ch - 'A';
    } else if (ch >= ' ') {
      r = (ch - ' ') % 26;
    } else {
      r = ch;
    }
    state->rotation[(i+1) % 4] = r;
  }
}

static void
dummy_prf_generate(void *state_, uint8_t *output, uint32_t idx)
{
  struct dummy_prf_state *state = state_;
  int i;
  const char *start = LOREM + 64*(idx & 7);

  for (i = 0; i < 64; i += 4) {
    output[i + 0] = rotate_char(start[i + 0], state->rotation[0]);
    output[i + 1] = rotate_char(start[i + 1], state->rotation[1]);
    output[i + 2] = rotate_char(start[i + 2], state->rotation[2]);
    output[i + 3] = rotate_char(start[i + 3], state->rotation[3]);
  }
}

struct ottery_prf dummy_prf =  {
  "LoremIpsum",
  "LoremIpsum",
  sizeof(struct dummy_prf_state), /* state_len */
  4, /* state_bytes */
  64, /* output_len */
  dummy_prf_setup,
  dummy_prf_generate
};

/* Assuming that we stir after every block, the first three blocks will
   be:

   "Nor again is there anyone who loves or pursues or desires to obt"
   "Nbf1atozn-wj gvvrr.rnlcee-kyo-zfvrg1oe.gueglef.fr-rvsvfvs-hf bpk"
   "Ebs%rtbne-jx1gijir!felpsv-xmf-mtmrt%fe!uletzvf!ti-ejjvsjj-ut1bcy"
 */
/* ================================================== */

char *state_allocation = NULL;
struct ottery_state *state = NULL;

#define OT_ENABLE_STATE TT_FIRST_USER_FLAG

/* XXXX this is mostly duplicate code. Can we make it redundant? */
void *
setup_state(const struct testcase_t *testcase)
{
  if (state)
    return NULL;
  struct ottery_config cfg;

  if (ottery_config_init(&cfg))
    return NULL;
  ottery_config_set_manual_prf_(&cfg, &dummy_prf);
  ottery_config_set_urandom_device_(&cfg, "/dev/zero");

  if (testcase->flags & OT_ENABLE_STATE) {
    state_allocation = malloc(ottery_get_sizeof_state() + 16);
    const int misalign = (int) (((uintptr_t)state_allocation) & 0xf);
    state = (struct ottery_state *)(state_allocation + ((16-misalign)&0xf));
    if (ottery_st_init(state, &cfg))
      return NULL;
  } else {
    if (ottery_init(&cfg))
      return NULL;
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

/* ================================================== */

static unsigned
get_u(const char *s)
{
  unsigned u;
  memcpy(&u, s, sizeof(u));
  return u;
}

static uint64_t
get_u64(const char *s)
{
  uint64_t u;
  memcpy(&u, s, sizeof(u));
  return u;
}

#if 0
static void
dump_u64(uint64_t u)
{
  char s[9];
  memcpy(s, &u, 8);
  s[8] = 0;
  printf("<<%s>>\n",s);
}
#endif

static void
test_uints(void *arg)
{
  (void) arg;
  /* XXXX Assuming sizeof(unsigned) == 4.  Must write a different unit
   * test wshen it isn't. */

  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("agai"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("n is"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u(" the"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("re anyon"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("e who lo"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("ves "));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("or pursu"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("es or de"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("sires to"));

  /* There are four bytes left in the output block, but RAND_UINT64 will throw
   * them out and start on the next block. */
  tt_assert(OTTERY_RAND_UINT64() == get_u64("atozn-wj"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64(" gvvrr.r"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("nlcee-ky"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("o-zfvrg1"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("oe.g"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("uegl"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("ef.f"));
  tt_assert(OTTERY_RAND_UINT64() == get_u64("r-rvsvfv"));
  /* This time, we consume the last of the buffer exactly: */
  tt_assert(OTTERY_RAND_UINT64() == get_u64("s-hf bpk"));

  /* And let's make sure that the next block starts right */
  tt_assert(OTTERY_RAND_UINT64() == get_u64("rtbne-jx"));
  tt_int_op(OTTERY_RAND_UNSIGNED(), ==, get_u("1gij"));

  /* Now let's make sure we like the state, if we can. */
  if (USING_STATE()) {
    struct ottery_state *st = STATE();
    struct dummy_prf_state *dst = (void*)st->state;
    tt_assert(0 == memcmp(st->buffer, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16));
    tt_int_op(0, ==, st->block_counter);
    tt_int_op(16, ==, st->pos);
    tt_int_op(5, ==, dst->rotation[0]);
    tt_int_op(4, ==, dst->rotation[1]);
    tt_int_op(1, ==, dst->rotation[2]);
    tt_int_op(18, ==, dst->rotation[3]);
  }

 end:
  ;
}

static void
test_add_seed(void *arg)
{
  (void)arg;
  uint8_t buf[21];

  tt_int_op(OTTERY_ADD_SEED((const uint8_t *)"Lorem ipsum", 11), ==, 0);

  /* It starts by dumping the next block, which starts "Nbf1".  Then we
   * xor in "Lore" and get [2, 13, 20, 84].  We set our key from this,
   * and get rotations [ 19, 2, 13, 20 ], so the next block starts
   * "Gqe4". Xor in "m ip" and get [42,81,12,68].  That produces rotations
   * of [3, 10, 16, 12].  The next block starts with "Qyh,"; xor in "sum"
   * and get [34,12,5,44].  Rotations are now [12, 2, 12, 5].  Now at
   * last generate a block:
   *     Zqd%mimnz"ux,vtjdg,fzaasq"ima"xthge%at,ugtezqu,td"pjekdje"ft,qny
   *
   * The first 4
   */
  OTTERY_RAND_BYTES(buf, 20);
  buf[20] = 0;
  tt_str_op((const char*)buf, ==, "mimnz\"ux,vtjdg,fzaas");

  if (USING_STATE()) {
    struct ottery_state *st = STATE();
    struct dummy_prf_state *dst = (void*)st->state;
    tt_assert(0 == memcmp(st->buffer, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0", 24));
    tt_int_op(0, ==, st->block_counter);
    tt_int_op(24, ==, st->pos);

    tt_int_op(5, ==, dst->rotation[0]);
    tt_int_op(25, ==, dst->rotation[1]);
    tt_int_op(16, ==, dst->rotation[2]);
    tt_int_op(3, ==, dst->rotation[3]);
  }

 end:
  ;
}

void
test_buf_short(void *arg)
{
  (void)arg;

  char buf[128];

  /* Try a short value that doesn't wrap. */
  OTTERY_RAND_BYTES((uint8_t*)buf, 16);
  buf[16] = 0;
  tt_str_op(buf, ==, "again is there a");

  /* Try a maximal value that can be fulfilled with a single reseed. */
  OTTERY_RAND_BYTES((uint8_t*)buf, 44 + 59);
  buf[44+59] = 0;
  tt_str_op(buf, ==,
            "nyone who loves or pursues or desires to obt"
            "atozn-wj gvvrr.rnlcee-kyo-zfvrg1oe.gueglef.fr-rvsvfvs-hf bp");

 end:
  ;
}

void
test_buf_long_1(void *arg)
{
  (void)arg;

  char buf[304];

  /* Try a short value that doesn't wrap. */
  OTTERY_RAND_BYTES((uint8_t*)buf, 16);
  buf[16] = 0;
  tt_str_op(buf, ==, "again is there a");

  /* Try a value too long to get with a single reseed. Make it use up
   * a bunch of the Lorem Ipsum text too. */
  OTTERY_RAND_BYTES((uint8_t*)buf, 303);
  buf[303] = 0;

  tt_str_op(buf, ==,
            "nyone who loves or pursues or desires to obt" /* 44 */
            /* Now we generate a bunch of blocks with the new key of "Nor " */
            "Nbf1atozn-wj gvvrr.rnlcee-kyo-zfvrg1oe.gueglef.fr-rvsvfvs-hf bpk" /*108*/
            "avb1pnwe bt1iggvls.1brqrufs1ig.zs-dria.1bhh1brqrufs1opqrsvceayzp" /*172*/
            " pwichajtnbtef.fcpii vb1wuwth-hfiy.rnq.gavb1cnb1pectues1hva1sbav" /*236*/
            " tfvag.glrojues1 Gc1tnyv n.krvjzay.vxnaglr.1wuwth-cw hg1eisi hbu" /*300*/
            /* next key is eehr, rotations are [17, 4 , 4, 7 ]. The last 3 bytes made
             * with this key are: */
            "krg" /*303*/);

  if (USING_STATE()) {
    struct ottery_state *st = STATE();
    struct dummy_prf_state *dst = (void*)st->state;
    tt_assert(0 == memcmp(st->buffer, "\0\0\0\0\0\0\0", 7));
    tt_int_op(0, ==, st->block_counter);
    tt_int_op(7, ==, st->pos);

    tt_int_op(17, ==, dst->rotation[0]);
    tt_int_op(4, ==, dst->rotation[1]);
    tt_int_op(4, ==, dst->rotation[2]);
    tt_int_op(7, ==, dst->rotation[3]);
  }

  OTTERY_RAND_BYTES((uint8_t*)buf, 57 + 10);
  buf[57+10] = 0;
  tt_str_op(buf, ==,
            "1lnpfrvcls-dyyfwtay.vxrftifs1 rltech1tb.fbgozn-gfmr.rdioe"
            "rkepe$mz1x");

 end:
  ;
}



struct testcase_t misc_tests[] = {
  END_OF_TESTCASES
};

#define COMMON_TESTS(flags)                                     \
  { "uints", test_uints, TT_FORK|flags, &setup, NULL },         \
  { "add_seed", test_add_seed, TT_FORK|flags, &setup, NULL },   \
  { "buf_short", test_buf_short, TT_FORK|flags, &setup, NULL }, \
  { "buf_long_1", test_buf_long_1, TT_FORK|flags, &setup, NULL }

struct testcase_t stateful_tests[] = {
  COMMON_TESTS(OT_ENABLE_STATE),
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
