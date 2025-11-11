# SAM

<img src="Dog/sam.svg" width=256 alt="logo">

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


## Installing and using SAM

To install SAM, you need Go:

```
go install github.com/rrthomas/sam@latest
```

Then run the `sam` command. To run a program, e.g.:

```
sam --wait screen_levy-c.yaml
```

Debugging output (with the `--debug` option) is shown in the terminal.

If you use the `--wait` option, and the program opens a window, then you
need to close the window to stop SAM at the end.

Documentation on the SAM virtual machine is in `SAM.md`.

See `HACKING.md` for information about developing SAM.


<!--  LocalWords:  homoiconically
 -->
