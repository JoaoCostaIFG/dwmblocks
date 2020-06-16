#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

pid_t
getdaemonpidof()
{
  /* check if other daemon is still running (despite file) */
  char cmd[40];
  snprintf(cmd, 40, "pidof -s -o %d dwmblocks", getpid());

  pid_t pid = 0;
  char buf[16];
  FILE* fp;
  if ((fp = popen(cmd, "r"))) {
    fgets(buf, sizeof(buf), fp);
    pid = strtoul(buf, NULL, 10);
    pclose(fp);
  }
  else
    die("getdaemonpidof:");

  return pid;
}

pid_t
getdaemonpid(char* pidfile)
{
  pid_t pid = 0;
  char buf[16];
  FILE* fp;
  if ((fp = fopen(pidfile, "r"))) { // try to find dwmblocks pid file
    fgets(buf, sizeof(buf), fp);
    pid = strtoul(buf, NULL, 10);
    fclose(fp);
  }
  else { // try to find a rogue process
    pid = getdaemonpidof();
    /* attempt to write found pid to file (which is missing) */
    if ((fp = fopen(pidfile, "a"))) {
      fprintf(fp, "%d", pid);
      fclose(fp);
    }
  }

  return pid;
}

void
writepidfile(char* pidfile)
{
  char buf[16];
  int fd = open(pidfile, O_CREAT | O_WRONLY | O_EXCL, 0640);
  if (fd < 0) {
    if (errno == EEXIST) {
      if (getdaemonpidof() != 0) // fail if there is a daemon already running
        die("writepidfile: There is a dwmblocks daemon currently running.");
      // reopen file (info is old/useless)
      if ((fd = open(pidfile, O_CREAT | O_WRONLY | O_TRUNC, 0640)) < 0)
        die("writepidfile:");
    }
  }

  snprintf(buf, 16, "%d", getpid());
  write(fd, buf, strlen(buf));
  close(fd);
}

void
daemonize(char* pidfile)
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

  /* create pid file */
  writepidfile(pidfile);

  /* close all open file descriptors (standard streams might suffice) */
  for (int fd = 3; fd < sysconf(_SC_OPEN_MAX); ++fd)
    close(fd);
  int devNull = open("/dev/null", O_RDWR);
  dup2(devNull, STDIN_FILENO);
  dup2(devNull, STDOUT_FILENO);
  dup2(devNull, STDERR_FILENO);
}
