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

#ifndef _WIN32
/* TODO: Support win32. */
#include <sys/socket.h>

/** DOCDOC */
static int
ottery_os_randbytes_egd(const struct ottery_osrng_config *cfg,
                        uint8_t *out, size_t outlen)
{
  int sock, n, result;
  unsigned char msg[2];

  if (! cfg || ! cfg->egd_sockaddr || ! cfg->egd_socklen)
    return OTTERY_ERR_INIT_STRONG_RNG;
  if (outlen > 255)
    return OTTERY_ERR_ACCESS_STRONG_RNG;

  sock = socket(cfg->egd_sockaddr->sa_family, SOCK_STREAM, 0);
  if (sock < 0)
    return OTTERY_ERR_INIT_STRONG_RNG;

  if (connect(sock, cfg->egd_sockaddr, cfg->egd_socklen) < 0) {
    result = OTTERY_ERR_INIT_STRONG_RNG;
    goto out;
  }

  msg[0] = 1;                      /* nonblocking request */
  msg[1] = (unsigned char) outlen; /* for outlen bytes */

  if (write(sock, msg, 2) != 2 ||
      read(sock, msg, 1)  != 1) {
    result = OTTERY_ERR_ACCESS_STRONG_RNG;
    goto out;
  }

  if (msg[0] != outlen) {
    /* TODO Use any bytes we get, even if they aren't as many as we wanted. */
    result = OTTERY_ERR_ACCESS_STRONG_RNG;
    goto out;
  }

  n = ottery_read_n_bytes_from_file_(sock, out, outlen);
  if (n < 0 || (size_t)n != outlen) {
    result = OTTERY_ERR_ACCESS_STRONG_RNG;
    goto out;
  }
  result = 0;
 out:
  close(sock);
  return result;
}

#define ENTROPY_SOURCE_EGD \
  { ottery_os_randbytes_egd, SRC(EGD)|FL(STRONG) }

#endif
