// Modify this file to change what commands output to your statusbar, and
// recompile using the make command.

static const Block blocks[] = {
  /* Icon    Command   Update Interval (s)   Update Signal (0 => disabled) */
  { "", "free -h | awk '/^Mem/ { print $3\"/\"$2 }' | sed s/i//g", 30, 0 },
  // { "", "printf '\na\n'", 30, 0 },

  { "", "sensors | awk '/^temp1:/{print $2}'", 30, 0 },

  { "", "date '+%H:%M'", 60, 0 },
};

// sets delimeter between status commands. NULL character ('\0') means no
// delimeter.
static char delim = ' ';
