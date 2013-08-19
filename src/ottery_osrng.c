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
#define OTTERY_INTERNAL
#include "ottery-internal.h"
#include "ottery.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#define RAND_WIN32
/** Generate random bytes using the Windows CryptGenRandom operating-system
 * RNG. */
static int
ottery_os_randbytes_win32(const struct ottery_osrng_config *cfg,
                          uint8_t *out, size_t outlen)
{
  /* On Windows, CryptGenRandom is supposed to be a well-seeded
   * cryptographically strong random number generator. */
  HCRYPTPROV provider;
  int retval = 0;
  (void) cfg;

  if (0 == CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT))
    return OTTERY_ERR_INIT_STRONG_RNG;

  if (0 == CryptGenRandom(provider, outlen, out))
    retval = OTTERY_ERR_ACCESS_STRONG_RNG;

  CryptReleaseContext(provider, 0);
  return retval;
}
#endif

#ifndef _WIN32
#define RAND_URANDOM
/** Generate random bytes using the unix-style /dev/urandom RNG, or another
 * such device as configured in the configuration. */
static int
ottery_os_randbytes_urandom(const struct ottery_osrng_config *cfg,
                            uint8_t *out, size_t outlen)
{
  /* On most unixes these days, you can get strong random numbers from
   * /dev/urandom.
   *
   * That's assuming that /dev/urandom is seeded.  For most applications,
   * that won't be a problem. But for stuff that starts close to system
   * startup, before the operating system has added any entropy to the pool,
   * it can be pretty bad.
   *
   * You could use /dev/random instead, if you want, but that has another
   * problem.  It will block if the OS PRNG has received less entropy than
   * it has emitted.  If we assume that the OS PRNG isn't cryptographically
   * weak, blocking in that case is simple overkill.
   *
   * It would be best if there were an alternative that blocked if the PRNG
   * had _never_ been seeded.  But most operating systems don't have that.
   */
  int fd;
  ssize_t n;
  int result = 0;
  const char *urandom_fname;
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
  if (cfg && cfg->urandom_fname)
    urandom_fname = cfg->urandom_fname;
  else
    urandom_fname = "/dev/urandom";

  fd = open(urandom_fname, O_RDONLY|O_CLOEXEC);
  if (fd < 0)
    return OTTERY_ERR_INIT_STRONG_RNG;
  if ((n = read(fd, out, outlen)) < 0 || (size_t)n != outlen)
    result = OTTERY_ERR_ACCESS_STRONG_RNG;
  close(fd);
  return result;
}
#endif

#if defined(i386) || \
    defined(__i386) || \
    defined(__x86_64) || \
    defined(__M_IX86) || \
    defined(_M_IX86) || \
    defined(__INTEL_COMPILER)
#define RAND_RDRAND
/** Helper: invoke the RDRAND instruction to get 4 random bytes in the output
 * value. Return 0 on success, and an error on failure. */
static int
rdrand(uint32_t *therand) {
 unsigned char status;
 asm volatile(".byte 0x0F, 0xC7, 0xF0 ; setc %1"
 : "=a" (*therand), "=qm" (status));
 return (status)==1 ? 0 : OTTERY_ERR_INIT_STRONG_RNG;
}

/** Generate bytes using the Intel RDRAND instruction. */
static int
ottery_os_randbytes_rdrand(const struct ottery_osrng_config *cfg,
                           uint8_t *out, size_t outlen)
{
  int err;
  uint32_t *up = (uint32_t *) out;
  (void) cfg;
  if (! (ottery_get_cpu_capabilities_() & OTTERY_CPUCAP_RAND))
    return OTTERY_ERR_INIT_STRONG_RNG;
  while (outlen >= 4) {
    if ((err = rdrand(up)))
      return err;
    up += 1;
    outlen -= 4;
  }
  if (outlen) {
    uint32_t tmp;
    if ((err = rdrand(&tmp)))
      return err;
    memcpy(up, &tmp, outlen);
  }
  return 0;
}
#endif

#define SRC(x) OTTERY_ENTROPY_SRC_ ## x
#define FL(x) OTTERY_ENTROPY_FL_ ## x
/** Table of RNG functions and their properties. */
static struct ottery_randbytes_source {
  int (*fn)(const struct ottery_osrng_config *, uint8_t *, size_t);
  uint32_t flags;
} RAND_SOURCES[] = {
#ifdef RAND_WIN32
  { ottery_os_randbytes_win32,   SRC(CRYPTGENRANDOM)|FL(OS)|FL(STRONG) },
#elif defined(RAND_URANDOM)
  { ottery_os_randbytes_urandom, SRC(RANDOMDEV)|FL(OS)|FL(STRONG) },
#endif
#ifdef RAND_RDRAND
  { ottery_os_randbytes_rdrand,  SRC(RDRAND)|FL(CPU)|FL(FAST)|FL(STRONG) },
#endif
  { NULL, 0 }
};
#undef SRC
#undef FL

int
ottery_os_randbytes_(const struct ottery_osrng_config *config,
                     uint32_t select_sources,
                     uint8_t *bytes, size_t n,
                     uint8_t *scratch, uint32_t *flags_out)
{
  int err = OTTERY_ERR_INIT_STRONG_RNG, i, last_err = 0;
  size_t j;
  uint32_t got = 0;
  const uint32_t disabled_sources = config ? config->disabled_sources : 0;

  memset(bytes, 0, n);

  *flags_out = 0;

  for (i=0; RAND_SOURCES[i].fn; ++i) {
    uint32_t flags = RAND_SOURCES[i].flags;
    /* Don't use a disabled source. */
    if (0 != (flags & disabled_sources))
      continue;
    /* If some flags must be set, only use those. */
    if ((flags & select_sources) != select_sources)
      continue;
    err = RAND_SOURCES[i].fn(config, scratch, n);
    if (err == 0) {
      got |= RAND_SOURCES[i].flags;
      /* XXXXX In theory, we should be combining these sources with a better
       * function than XOR. XOR is good enough if they're independent, though.
       */
      for (j = 0; j < n; ++j) {
        bytes[j] ^= scratch[j];
      }
    } else {
      last_err = err;
    }
  }

  /* Do not report success unless at least one source was strong. */
  if (0 == (got & OTTERY_ENTROPY_FL_STRONG))
    return last_err ? last_err : OTTERY_ERR_INIT_STRONG_RNG;

  /* Wipe the scratch space */
  ottery_memclear_(scratch, n);

  *flags_out = got;
  return 0;
}
