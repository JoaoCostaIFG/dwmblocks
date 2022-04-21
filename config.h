// Modify this file to change what commands output to your statusbar, and
// recompile using the make command.

/* name of the environment variable that will be set on button input */
#define DWMBLOCKS_ENV "DWMBLOCKS_BUTTON"
#define DWMBLOCKS_PIDFILEPATH "/tmp/dwmblocks.pid"

static const Block blocks[] = {
  /*Icon*/  /*Command*/       /*Update Interval (s)*/ /*Update Signal*/
  { "",       "dwmb-bat",       60,                     5 },
  { "",       "dwmb-temps",     60,                     1 },
  { "",       "dwmb-volume",    0,                      2 },
  { "",       "dwmb-clock",     30,                     3 },
  { "♥",      "dwmb-heart",     0,                      4 },
};

// sets delimeter between status commands. NULL character ('\0') means no
// delimeter.
static char delim = ' ';
static char replaceNewLineChar = ' '; // '\0' for deletion
