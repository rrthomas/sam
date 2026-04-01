// Sam SDL traps.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_SDL
#define SAM_SDL

#include "sam.h"

#define SAM_DISPLAY_WIDTH 1024
#define SAM_DISPLAY_HEIGHT 720
extern unsigned sam_update_interval;

typedef struct sam_audiofile sam_audiofile_t;

sam_word_t sam_sdl_init(void);
void sam_sdl_finish(void);
int sam_sdl_process_events(void);
int sam_sdl_window_used(void);
uint32_t sam_getpixel(int x, int y);
void sam_dump_screen(const char *filename);
void sam_update_screen(void);
void sam_sdl_wait(void);

#endif
