// Sam registers and their C types.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

R(ssize, sam_uword_t) // Size of stack in words
R(sp, sam_uword_t) // Number of words (NOT items!) in current stack
R(pc, sam_uword_t) // Address of next instruction to execute
R(pc0, sam_uword_t) // Start of current routine
