# dwmblocks

Modular status bar for dwm written in c.

## Modifying blocks

The status bar is made from text output from command-line programs.
Blocks are added and removed by editing the **config.h** header file.

## Mouse clicks

If you have the appropriate [patches](https://dwm.suckless.org/patches/statuscmd/)
in your DWM build, you _blocks_ can handle mouse clicks. The code those patches
introduce in this project has been slightly modified from the original.

Credits for the original patches to: **Daniel Bylinka - daniel.bylinka@gmail.com**

### Environment variable

By default, the enviroment variable that is set for mouse button events is
**DWMBLOCKS_BUTTON**. You can change this variable in _config.h_, but remember
to also change it in your **dwm** build.

## Credits

[Original](https://github.com/torrinfail/dwmblocks/) was written by
[torrinfail](https://github.com/torrinfail).
