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
 * @file fake_egd.c
 *
 * A broken EGD implementation that works just well enough to respond to
 * the "nonblocking request" request type and return (NON-RANDOM!)
 * prefixes of bits of "O Fortuna".
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static const unsigned char o_fortuna[] =
  "Sors immanis et inanis rota tu volubilis status malus vana salus "
  "semper dissolubilis obumbrata et velata michi quoque niteris nunc "
  "per ludum dorsum nudum fero tui sceleris Sors salutis et virtutis "
  "michi nunc contraria est affectus et defectus semper in angaria";

int bug_truncate_output = 0;
int bug_short_output = 0;
int bug_no_output = 0;
int bug_close_after_read = 0;
int bug_close_before_read = 0;

static int
reply(int fd)
{
  unsigned char len;
  unsigned char buf[257];
  int n;

  if (bug_close_before_read)
    return 0;

  if (read(fd, buf, 2) != 2)
    return -1;
  if (bug_close_after_read)
    return 0;

  if (buf[0] != 1)
    return -2;
  len = buf[1];

  if (bug_short_output)
    len /= 2;
  if (bug_no_output)
    len = 0;

  buf[0] = len;
  memcpy(buf+1, o_fortuna, len);
  n = (int)len + 1;
  if (bug_truncate_output)
    n /= 2;

  if (write(fd, buf, n) != n)
    return -3;

  return 1;
}

struct bug_table_ent {
  const char *name;
  int *flag;
} bug_table[] = {
  { "--truncate-output",   &bug_truncate_output },
  { "--short-output",      &bug_short_output },
  { "--no-output",         &bug_no_output },
  { "--close-after-read",  &bug_close_after_read },
  { "--close-before-read", &bug_close_before_read },
  { NULL, NULL }
};

static int
option(const char *opt)
{
  int i;
  for (i = 0; bug_table[i].name; ++i) {
    if (!strcmp(opt, bug_table[i].name)) {
      *bug_table[i].flag = 1;
      return 0;
    }
  }
  return -1;
}

static int
getport(const char *opt, int *num)
{
  char *endp;
  long p = strtol(opt, &endp, 10);
  if (p < 0 || p > 65535 || *endp)
    return -1;

  *num = (int)p;
  return 0;
}

int
main(int argc, char **argv)
{
  int i;
  int n_addrs = 0;
  int port = 0;
  socklen_t socklen;
  int family;
  int listener;
  int fd;
  const char *path = NULL;
  struct sockaddr_in sin;
  struct sockaddr_un sun;
  struct sockaddr *sa;

  for (i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (option(argv[i]) < 0) {
        printf("Unrecognized option %s\n", argv[i]);
        return 1;
      }
    }
    if (getport(argv[i], &port) == 0) {
      ++n_addrs;
    } else {
      path = argv[i];
      ++n_addrs;
    }
  }
  if (n_addrs > 1) {
    printf("Too many arguments\n");
    return 1;
  }

  if (path) {
    memset(&sun, 0, sizeof(sun));
    family = AF_UNIX;
    socklen = sizeof(sun);
    sun.sun_family = AF_UNIX;
    if (strlen(path)+1 > sizeof(sun.sun_path))
      return 1;
    memcpy(sun.sun_path, path, strlen(path)+1);
    sa = (void *)&sun;
  } else {
    memset(&sin, 0, sizeof(sin));
    family = AF_INET;
    socklen = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0x7f000001);
    sin.sin_port = htons(port);
    sa = (void*)&sin;
  }

  listener = socket(family, SOCK_STREAM, 0);
  if (listener < 0) {
    perror("socket");
    return 1;
  }

  if (bind(listener, sa, socklen) < 0) {
    perror("bind");
    return 1;
  }
  if (path == 0) {
    socklen = sizeof(sin);
    if (getsockname(listener, (struct sockaddr*)&sin, &socklen) < 0) {
      perror("getsockname");
    } else {
      printf("PORT:%d\n", ntohs(sin.sin_port));
    }
  }

  if (listen(listener, 16) < 0) {
    perror("listen");
    return 1;
  }

  fd = accept(listener, sa, &socklen);
  if (fd < 0) {
    perror("accept");
    return 1;
  }

  if (reply(fd) < 0)
    return 1;

  close(fd);
  close(listener);

  return 0;
}


