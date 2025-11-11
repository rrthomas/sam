# Hacking on SAM

The main sources are in the directory libsam. These are written in C. The
front-end is written in Go, and its sources are in the top-level directory.
It does not need the C library to be installed.

The mascot files are in the directory `Dog`.


## Building SAM

There are two different ways to build SAM. In any case, you’ll need Go and a
C compiler, as well as SDL2 and SDL2_gfx (on Debian-compatible systems,
install package `libsdl2-gfx-dev`).

For more precise examples of building on various operating systems, see
`.github/workflows/ci.yml`.

To simply run SAM via its front end, use Go:

```
go run .
```

The autotools build system is used to build and install the C library and
run the tests. To use this, you’ll need automake, autoconf, and libtool.

To be able to build the library and run the tests:

```
autoreconf -fi
./configure
```

Then to build the library, `make`, and to run the tests, `make check`.
