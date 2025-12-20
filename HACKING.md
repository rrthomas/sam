# Hacking on SAM

SAM consists of a C library and Go front-end. The library may be used
independently of the front-end; in particular, it should be more portable.

The library sources are in the directory libsam. The front-end sources are
in the top-level directory, and building it statically links the C library
into it.

The mascot files are in the directory `mascot`.


## Building SAM

There are two ways to build SAM.

In any case, you’ll need Go and a C compiler, as well as SDL2 and SDL2_gfx
(on Debian-compatible systems, install package `libsdl2-gfx-dev`).

### Building the front-end

This is done with the `go` command. There’s no need to build the program
explicitly; you can directly build & run the front end:

```
go run .
```

### Building the C library

The autotools build system is used to build and install the C library and
run the tests. To use this, you’ll need automake, autoconf, and libtool.

To build the library and run the tests:

```
autoreconf -fi
./configure
make
make check
```

For more detailed information on building the library on various operating
systems, see `.github/workflows/ci.yml`.
