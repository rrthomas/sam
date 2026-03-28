// Implementation-private declarations for SDL module.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <SDL_mixer.h>

// Structs
typedef struct sam_audiofile {
    Mix_Music *audio;
} sam_audiofile_t;
