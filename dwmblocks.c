#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

#define LENGTH(X)     (sizeof(X) / sizeof(X[0]))
#define CMDLENGTH     40
#define OVERWRITE_ENV 1

// TODO
// If sig is 0, all fields shall be updated

typedef struct
{
  char* icon;
  char* command;
  unsigned int interval;
  unsigned int signal;
} Block;
size_t replace_str_char(char* str, char to_replace, char new_char);
void sighandler(int num);
void buttonhandler(int sig, siginfo_t* si, void* ucontext);
void getcmds(int time);
#ifndef __OpenBSD__
void getsigcmds(int signal);
void setupsignals();
void sighandler(int signum);
#endif
int getstatus(char* str, char* last);
void setroot();
void statusloop();
static void terminate(const int signo);
void daemonize();

#include "config.h"

static Display* dpy;
static char statusbar[LENGTH(blocks)][CMDLENGTH] = { 0 };
static char statusstr[2][256];
static int statusContinue      = 1;
static void (*writestatus)()   = setroot;
static const size_t blocks_len = LENGTH(blocks);
static char button[]           = "\0";

static void
usage(void)
{
  die("usage: dwmblocks [-pb] [-d<c>] [-S<i>]");
}

static void
terminate(const int signo)
{
  if (statusContinue)
    statusContinue = 0;
  else
    exit(1);
}

/* replaces all occurences of given char in string with the new one
 * pass '\0' as new for char deletion. Return new str size.
 */
size_t
replace_str_char(char* str, char to_replace, char new_char)
{
  char* write = str;

  // get rid of starting repalce char
  if (*str == to_replace)
    ++str;

  size_t size = 0;
  while (*str != '\0') {
    // take care of unwanted char
    if (*str == to_replace) {
      // delete char if it's a trailing char or we're told to delete it
      if (*(str + 1) == '\0')
        break;
      else if (new_char == '\0')
        ++str;
      else
        *str = new_char;
    }
    *(write++) = *(str++);
    ++size;
  }

  *write = '\0';
  return size;
}

/* opens process *cmd and stores output in *output */
void
getcmd(const Block* block, char* output)
{
  if (block->signal) // prepend signal number if signal defined for cmd
    *(output++) = block->signal;
  strcpy(output, block->icon);

  char* cmd = block->command;
  FILE* cmdf;
  if (*button) {
    setenv(DWMBLOCKS_ENV, button, OVERWRITE_ENV);
    cmdf    = popen(cmd, "r");
    *button = '\0';
    unsetenv(DWMBLOCKS_ENV);
  }
  else {
    cmdf = popen(cmd, "r");
  }
  if (!cmdf) // fork/pipe failed
    return;

  int i = strlen(block->icon);
  i += fread(output + i, sizeof(char), CMDLENGTH - i, cmdf);
  output[i] = '\0'; // NULL terminate read bytes

  i = replace_str_char(output, '\n', replaceNewLineChar);
  if (delim != '\0' && i) {
    output[i++] = delim;
    output[i]   = '\0';
  }

  pclose(cmdf);
}

void
getcmds(int time)
{
  const Block* current;
  for (int i = 0; i < blocks_len; ++i) {
    current = blocks + i;
    if ((current->interval != 0 && time % current->interval == 0) || time == -1)
      getcmd(current, statusbar[i]);
  }
}

#ifndef __OpenBSD__
void
getsigcmds(int signal)
{
  const Block* current;
  for (int i = 0; i < blocks_len; i++) {
    current = blocks + i;
    if (current->signal == signal)
      getcmd(current, statusbar[i]);
  }
}

void
setupsignals()
{
  struct sigaction sa, button_sa;
  sigemptyset(&sa.sa_mask);
  sigemptyset(&button_sa.sa_mask);

  sa.sa_handler = sighandler;
  sa.sa_flags   = SA_RESTART;

  for (int i = 0; i < blocks_len; i++) {
    if (blocks[i].signal <= 0) // unhandled sig
      break;
    // ignore other signals when handling SIGUSR1
    sigaddset(&button_sa.sa_mask, SIGRTMIN + blocks[i].signal);
    if (sigaction(SIGRTMIN + blocks[i].signal, &sa, NULL))
      perror("Failed setting signal from config.");
  }

  button_sa.sa_sigaction = buttonhandler;
  button_sa.sa_flags     = SA_RESTART | SA_SIGINFO;
  sigaction(SIGUSR1, &button_sa, NULL);
}
#endif

int
getstatus(char* str, char* last)
{
  strcpy(last, str);
  str[0] = '\0';
  for (int i = 0; i < blocks_len; i++)
    strcat(str, statusbar[i]);
  str[strlen(str) - 1] = '\0';
  return strcmp(str, last); // 0 if they are the same
}

void
setroot()
{
  // Only set root if text has changed.
  if (!getstatus(statusstr[0], statusstr[1]))
    return;

  if (XStoreName(dpy, DefaultRootWindow(dpy), statusstr[0]) < 0)
    die("XStoreName: Allocation failed");
  XFlush(dpy);
}

void
pstdout()
{
  // Only write out if text has changed.
  if (!getstatus(statusstr[0], statusstr[1]))
    return;

  printf("%s\n", statusstr[0]);
  fflush(stdout);
}

void
statusloop()
{
#ifndef __OpenBSD__
  setupsignals();
#endif
  getcmds(-1); // force run of all cmds

  unsigned i = 0; // restarts from 0 after max unsigned
  while (statusContinue) {
    getcmds(i);
    writestatus();
    sleep(1.0);
    ++i;
  }

  /* delete PID file */
  unlink(DWMBLOCKS_PIDFILEPATH);
  /* clear status on exit */
  XStoreName(dpy, DefaultRootWindow(dpy), NULL);
  if (XCloseDisplay(dpy) < 0)
    die("XCloseDisplay: Failed to close display");
}

#ifndef __OpenBSD__
void
sighandler(int signum)
{
  getsigcmds(signum - SIGRTMIN);
  writestatus();
}

void
buttonhandler(int sig, siginfo_t* si, void* ucontext)
{
  *button = ('0' + si->si_value.sival_int) & 0xff;
  getsigcmds(si->si_value.sival_int >> 8);
  writestatus();
}
#endif

int
main(int argc, char** argv)
{
  /*
   * -b daemonizes process
   * -d sets the field delimiter
   * -p printf to stdout
   * -s send signal to running daemon
   */
  unsigned short int bg = 0;
  int s                 = 0;
  ARGBEGIN
  {
    case 'b':
      bg = 1;
      break;
    case 'd':
      delim = (*argv)[++i];
      break;
    case 'p':
      writestatus = pstdout;
      break;
    case 's':
      s = strtol((*argv) + i + 1, NULL, 10);
      for (; (*argv)[i + 1]; ++i) {
      }
      break;
    default:
      usage();
      break;
  }
  ARGEND
  if (argc)
    usage();

  if (s) { // signal daemon and quit
    pid_t daemonpid;
    if ((daemonpid = getdaemonpid(DWMBLOCKS_PIDFILEPATH)))
      kill(daemonpid, SIGRTMIN + s);
    else
      die("dwmblocks: Couldn't find a running deamon.");

    exit(0);
  }

  if (bg)
    daemonize(DWMBLOCKS_PIDFILEPATH);

  /* termination handlers */
  struct sigaction sa;
  sa.sa_handler = terminate;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGINT);
  sigaddset(&sa.sa_mask, SIGTERM);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa, NULL) || sigaction(SIGTERM, &sa, NULL))
    die("Term signals setting failed:");

  /* open display */
  if (!(dpy = XOpenDisplay(NULL)))
    die("XOpenDisplay: Failed to open display.");
  statusloop();

  return 0;
}
