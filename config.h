// Modify this file to change what commands output to your statusbar, and
// recompile using the make command.

#define DWMBLOCKS_ENV "DWMBLOCKS_BUTTON"

static const Block blocks[] = {
  /* Icon    Command   Update Interval (s)   Update Signal (0 => disabled) */
  { "",   "cpu",                        300,  1 },
  { "",   "date '+%a %d-%m-%y %H:%M'",  60,   0 },
  { "♥",  "",                           0,    0 },
};

// sets delimeter between status commands. NULL character ('\0') means no
// delimeter.
static char delim = ' ';
