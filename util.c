#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"

static void
verr(const char* fmt, va_list ap)
{
  vfprintf(stderr, fmt, ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  }
  else {
    fputc('\n', stderr);
  }
}

void
warn(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  verr(fmt, ap);
  va_end(ap);
}

void
die(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  verr(fmt, ap);
  va_end(ap);

  exit(1);
}

void
daemonize()
{
  /*
   * Stevens' "Advanced Programming in the UNIX Environment" for details (ISBN
   * 0201563177) http://www.erlenstar.demon.co.uk/unix/faq_2.html#SEC16
   */

  /* first fork (grantee that we're not a session leader) */
  int pid = fork();
  switch (pid) {
    case -1:
      die("daemonize: first fork failed.");
      break;
    case 0:
      break;
    default:
      _exit(0);
      break;
  }

  /* setsid to detach from invoking command line/shell (fails if we're a session
   * leader) */
  if (setsid() < 0)
    die("daemonize: setsid failed.");

  /* second fork (grantee we're not the session leader so we can't require
     a controlling terminal) */
  pid = fork();
  switch (pid) {
    case -1:
      die("daemonize: second fork failed.");
      break;
    case 0:
      break;
    default:
      _exit(0);
      break;
  }

  /* chdir to root so we don't keep a directory in use */
  if (chdir("/") < 0)
    die("daemonize: chdir to '/' failed.");
  /* optionally set umask(0) */

  /* close all open file descriptors (standard streams might suffice) */
  for (int fd = 3; fd < sysconf(_SC_OPEN_MAX); ++fd)
    close(fd);
  int devNull = open("/dev/null", O_RDWR);
  dup2(devNull, STDIN_FILENO);
  dup2(devNull, STDOUT_FILENO);
  dup2(devNull, STDERR_FILENO);
}
