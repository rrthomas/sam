// Sam sound traps.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_TRAP_AUDIO
#define SAM_TRAP_AUDIO

#include "sam.h"

sam_word_t sam_audio_trap(sam_state_t *state, sam_uword_t function);
char *sam_audio_trap_name(sam_word_t function);

#define SAM_TRAP_AUDIO_BASE 0x500

enum SAM_TRAP_AUDIO {
  TRAP_AUDIO_NEW_AUDIOFILE = SAM_TRAP_AUDIO_BASE,
  TRAP_AUDIO_VOL,
  TRAP_AUDIO_PITCH,
  TRAP_AUDIO_CUE,
  TRAP_AUDIO_PLAY,
  TRAP_AUDIO_DURATION,
  TRAP_AUDIO_ISPLAYING,
  TRAP_AUDIO_JUMP,
  TRAP_AUDIO_LOOP,
  TRAP_AUDIO_PAN,
  TRAP_AUDIO_PAUSE,
  TRAP_AUDIO_SPEED,

  TRAP_AUDIO_APPLAUSE,
  TRAP_AUDIO_BEEP,
  TRAP_AUDIO_BELL,
  TRAP_AUDIO_COW,
  TRAP_AUDIO_EXPLOSION,
  TRAP_AUDIO_GONG,
  TRAP_AUDIO_HORSE,
  TRAP_AUDIO_LASER,
  TRAP_AUDIO_OOPS,
};

#endif
