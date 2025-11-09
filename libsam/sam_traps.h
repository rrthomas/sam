// The trap opcodes.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

// Origin is 0,0 at top left
enum {
      TRAP_BLACK,
      TRAP_WHITE,
      TRAP_DISPLAY_WIDTH,
      TRAP_DISPLAY_HEIGHT,
      TRAP_CLEARSCREEN,
      TRAP_SETDOT,
      TRAP_DRAWLINE,
      TRAP_DRAWRECT,
      TRAP_DRAWROUNDRECT,
      TRAP_FILLRECT,
      TRAP_DRAWCIRCLE,
      TRAP_FILLCIRCLE,
      TRAP_DRAWBITMAP,
      // TODO: Sound
      // TDOO: Input: joystick, buttons
};
