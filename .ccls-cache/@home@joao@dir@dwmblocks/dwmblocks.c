#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define LENGTH(X) (sizeof(X) / sizeof(X[0]))
#define CMDLENGTH 40 // TODO this might be problematic (move to config ?)

typedef struct
{
  char* icon;
  char* command;
  unsigned int interval;
  unsigned int signal;
} Block;
void sighandler(int num);
void remove_str_char(char* str, char c);
void getcmds(int time);
#ifndef __OpenBSD__
void getsigcmds(int signal);
void setupsignals();
void sighandler(int signum);
#endif
int getstatus(char* str, char* last);
void setroot();
void statusloop();
void termhandler(int signum);

#include "config.h"

// static Display* dpy;
static int screen;
static Window root;
static char statusbar[LENGTH(blocks)][CMDLENGTH] = { 0 };
static char statusstr[2][256];
static int statusContinue    = 1;
static void (*writestatus)() = setroot;

void
remove_str_char(char* str, char c)
{
  char* k = str;
  char* write = str;
  while (*str != '\0') {
    if (*str == c) { // skip unwanted char
      ++str;
      *write = *str;
    }
    ++str;
    ++write;
  }
}

/* opens process *cmd and stores output in *output */
void
getcmd(const Block* block, char* output)
{
  char* cmd  = block->command;
  FILE* cmdf = popen(cmd, "r");
  if (!cmdf) { // clear cmd if forking fails
    sprintf(output, "%s--", block->icon);
    return;
  }

  char c;
  int i = strlen(block->icon);
  strcpy(output, block->icon);
  fgets(output + i, CMDLENGTH - i, cmdf);
  /* fread(output + i, size_t __size, size_t __n, FILE *restrict __stream) */
  /* remove_str_char(output, '\n'); */
  i = strlen(output);
  if (delim != '\0' && --i)
    output[i++] = delim;
  output[i++] = '\0';
  pclose(cmdf);
}

void
getcmds(int time)
{
  const Block* current;
  for (int i = 0; i < LENGTH(blocks); ++i) {
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
  for (int i = 0; i < LENGTH(blocks); i++) {
    current = blocks + i;
    if (current->signal == signal)
      getcmd(current, statusbar[i]);
  }
}

void
setupsignals()
{
  struct sigaction sa;
  sa.sa_handler = sighandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  for (int i = 0; i < LENGTH(blocks); i++) {
    if (blocks[i].signal <= 0)
      break;
    if (sigaction(SIGRTMIN + blocks[i].signal, &sa, NULL))
      perror("Failed setting signal from config");
  }
}
#endif

int
getstatus(char* str, char* last)
{
  strcpy(last, str);
  str[0] = '\0';
  for (int i = 0; i < LENGTH(blocks); i++)
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
  Display* dpy;
  if (!(dpy = XOpenDisplay(NULL)))
    return;

  screen = DefaultScreen(dpy);
  root   = RootWindow(dpy, screen);
  XStoreName(dpy, root, statusstr[0]);
  XCloseDisplay(dpy);
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
}

#ifndef __OpenBSD__
void
sighandler(int signum)
{
  getsigcmds(signum - SIGRTMIN);
  writestatus();
}
#endif

void
termhandler(int signum)
{
  statusContinue = 0;
  exit(0);
}

int
main(int argc, char** argv)
{
  for (int i = 0; i < argc; i++) { // -d set delimiter
    if (!strcmp("-d", argv[i]))
      delim = argv[++i][0];
    else if (!strcmp("-p", argv[i])) // -p to print to stdout
      writestatus = pstdout;
  }

  // cleanup handler
  struct sigaction sa;
  sa.sa_handler = termhandler;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGINT);
  sigaddset(&sa.sa_mask, SIGTERM);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) || sigaction(SIGTERM, &sa, NULL)) {
    perror("Term signals setting failed");
    exit(1);
  }

  statusloop();
  exit(0);
}
