// Sam graphics traps.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_TRAP_GRAPHICS
#define SAM_TRAP_GRAPHICS

#define SAM_DISPLAY_WIDTH 800
#define SAM_DISPLAY_HEIGHT 600
extern const unsigned sam_update_interval;

sam_word_t sam_graphics_init(void);
void sam_graphics_finish(void);
int sam_graphics_process_events(void);
sam_word_t sam_graphics_trap(sam_uword_t function);
char *sam_graphics_trap_name(sam_word_t function);
int sam_graphics_window_used(void);
uint32_t sam_getpixel(int x, int y);
void sam_dump_screen(const char *filename);
void sam_update_screen(void);
void sam_graphics_wait(void);

#define SAM_TRAP_GRAPHICS_BASE 0x200

enum SAM_TRAP_GRAPHICS {
  // Graphics
  // Origin 0,0 is at top left.
  TRAP_GRAPHICS_BLACK = SAM_TRAP_GRAPHICS_BASE,
  TRAP_GRAPHICS_WHITE,
  TRAP_GRAPHICS_DISPLAY_WIDTH,
  TRAP_GRAPHICS_DISPLAY_HEIGHT,
  TRAP_GRAPHICS_CLEARSCREEN,
  TRAP_GRAPHICS_SETDOT,
  TRAP_GRAPHICS_DRAWLINE,
  TRAP_GRAPHICS_DRAWRECT,
  TRAP_GRAPHICS_DRAWROUNDRECT,
  TRAP_GRAPHICS_FILLRECT,
  TRAP_GRAPHICS_DRAWCIRCLE,
  TRAP_GRAPHICS_FILLCIRCLE,
  TRAP_GRAPHICS_DRAWBITMAP,
};

#endif
