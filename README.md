# SAM

<img src="Dog/noun_Dog_79221.svg" width=64 alt="logo">

SAM is a Simple Abstract Machine intended as a teaching tool and toy.

Sam is a Stack–Array Machine. It has no memory or registers; instead it has
a nested stack of stacks. Each stack is an array of items that can be
executed.

SAM was designed for the TiLDA Mke badge (whose hardware seems to be
codenamed “sam”), an Arduino Due-compatible device designed for
Electromagnetic Field 2014. It now runs on most personal computers.

SAM was invented as a tenth birthday present for my nephew, Sam. It was
named after my first computer, a Sinclair ZX81, “Super Advanced Micro”.
Sam’s logo is a simpatico astute mongrel called Sam.


## The idea

SAM is a simple, concrete system for teaching programming. It is low-level
enough that it exposes most of the fundamental issues of dealing with
computers, and high level enough that one can comfortably program in its
machine code. It has only one sort of state—the stack—to understand, which
can be inspected in its entirety, and corresponds homoiconically to the
program and state. It can be interrupted and resumed, and is easy to study
and reason about.


## The code

The SAM-specific files are in `Mk2-Firmware/EMF2014`. SAM itself is in the
source files `sam*`. The logo files are in the directory `Dog`.


## Building SAM

To build you will need Go, a C compiler, and SDL2 (on Debian-compatible
systems, install package `libsdl2-gfx-dev`).

To compile and run a program:

```
go run . --wait screen_levy-c.yaml
```

Debugging output is shown in the terminal. If a window has opened, close it
to end the program.

To be able to build the documentation and run the tests:

```
autoreconf -fi
./configure
```

Then to build the documentation: `make`, and to run the tests, `make check`.

<!--  LocalWords:  homoiconically
 -->
