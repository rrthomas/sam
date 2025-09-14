# SAM

<img src="EMF2014/Dog/noun_Dog_79221.svg" width=64 alt="logo">

SAM is a Simple Abstract Machine intended as a teaching tool and toy.

Sam is a Stack–Array Machine. It has no memory or registers; instead it has
a nested stack of stacks. Each stack is an array of items that can be
executed.

SAM was designed for the TiLDA Mke badge (whose hardware seems to be
codenamed “sam”), an Arduino Due-compatible device designed for
Electromagnetic Field 2014. It now runs on most personal computers (it needs a C compiler and SDL2).

SAM was invented as a tenth birthday present for my nephew, Sam. It was
named after my first computer, a Sinclair ZX81, “Super Advanced Micro”.
Sam’s logo is a simpatico astute mongrel called Sam.


## The idea

SAM is a simple, concrete system for teaching programming. It is low-level enough that it exposes most of the fundamental issues of dealing with computers, and high level enough that one can comfortably program in its machine code. It has only one sort of state—the stack—to understand, which can be inspected in its entirety, and corresponds homoiconically to the program and state. It can be interrupted and resumed, and is easy to study and reason about.


## The code

The SAM-specific files are in `Mk2-Firmware/EMF2014`. SAM itself is in the
source files `sam*`. The logo files are in the directory `Dog`. (Some of the
C files end in `.x` so they are not automatically built by the Arduino IDE.)


## Building SAM

To build on a GNU-compatible system you will need GCC, Python 3 (for the
`samc` assembler) with pyyaml and pyyaml-include, and SDL2 (on
Ubuntu-compatible systems, install package `libsdl2-gfx-dev:i386`). Run the
script `mksam` with the desired code, and then run the resulting `sam`
binary; for example:

```
./mksam graphics.yaml
./sam
```

Debugging output is shown in the terminal. Press Return in the terminal to
close the window and end the program.

<!--  LocalWords:  homoiconically
 -->
