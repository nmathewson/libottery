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

int
ottery_os_randbytes_(const char *urandom_fname, uint8_t *out, size_t outlen)
{
#ifdef _WIN32
  (void)urandom_fname;
  /* On Windows, CryptGenRandom is supposed to be a well-seeded
   * cryptographically strong random number generator. */
  HCRYPTPROV provider;
  int retval = 0;
  if (0 == CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT))
    return OTTERY_ERR_INIT_STRONG_RNG;

  if (0 == CryptGenRandom(provider, outlen, out))
    retval = OTTERY_ERR_ACCESS_STRONG_RNG;

  CryptReleaseContext(provider, 0);
  return retval;
#else
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
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
  if (!urandom_fname)
    urandom_fname = "/dev/urandom";
  fd = open(urandom_fname, O_RDONLY|O_CLOEXEC);
  if (fd < 0)
    return OTTERY_ERR_INIT_STRONG_RNG;
  if ((n = read(fd, out, outlen)) < 0 || (size_t)n != outlen)
    result = OTTERY_ERR_ACCESS_STRONG_RNG;
  close(fd);
  return result;
#endif
}

