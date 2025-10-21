// The trap opcodes.
//
// (c) Reuben Thomas 2020
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
      TRAP_DRAWLINE, // Draws both end-points
      // Rectangles are at x,y and are w×h pixels in size
      TRAP_DRAWRECT,
      // Radius between 0 and MIN(w, h)/2 inclusive
      TRAP_DRAWROUNDRECT,
      TRAP_FILLRECT,
      TRAP_INVERTRECT,
      // Circles have their centre at x,y and are radius+1 pixels in diameter
      TRAP_DRAWCIRCLE,
      TRAP_FILLCIRCLE,
      TRAP_DRAWBITMAP,
      //LEDs
      //Sound
      //Input: joystick, buttons
};
