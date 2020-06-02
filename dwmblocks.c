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
void replace_str_char(char* str, char to_replace, char new_char);
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

static char statusbar[LENGTH(blocks)][CMDLENGTH] = { 0 };
static char statusstr[2][256];
static int statusContinue    = 1;
static void (*writestatus)() = setroot;

/* replaces all occurences of given char in string with the new one
 * pass '\0' as new for char deletion
 */
void
replace_str_char(char* str, char to_replace, char new_char)
{
  char* write = str;
  while (*str != '\0') {
    // take care of unwanted char
    if (*str == to_replace) {
      // delete char if it's a trailing new-line or we're told to delete it
      if (new_char == '\0' || *(str + 1) == '\0')
        ++str;
      else
        *str = new_char;
    }
    *(write++) = *(str++);
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
  fread(output + i, sizeof(char), CMDLENGTH - i, cmdf);
  replace_str_char(output, '\n', ' ');
  i = strlen(output);
  if (delim != '\0' && i)
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

  int screen  = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);
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
