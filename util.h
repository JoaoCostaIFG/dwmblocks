#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

#define ARGBEGIN                                 \
  for (*argv ? (--argc, ++argv) : (void*)0;      \
       *argv && (*argv)[0] == '-' && (*argv)[1]; \
       --argc, ++argv) {                         \
    if ((*argv)[1] == '-' && !(*argv)[2]) {      \
      argc--, argv++;                            \
      break;                                     \
    }                                            \
    for (int i = 1; (*argv)[i]; ++i) {           \
      switch ((*argv)[i])
#define ARGEND \
  }            \
  }

void warn(const char* fmt, ...);

void die(const char* fmt, ...);

pid_t getdaemonpid();

void daemonize();

#endif // UTIL_H
