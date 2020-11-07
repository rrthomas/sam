// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

// Optimization
// Hint that `x` is usually true/false.
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#if 1 == 1
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif


// Errors
#define THROW(code)                             \
    do {                                        \
        error = code;                           \
        goto error;                             \
    } while (0)

#define THROW_IF_ERROR(code)                    \
    if (code != SAM_ERROR_OK)                   \
        THROW(code)


// Stack access
#define POP(ptr)                                                        \
    THROW_IF_ERROR(sam_pop_stack(sam_s0, sam_ssize, &sam_sp, ptr))
#define PUSH(v)                                                         \
    do {                                                                \
        THROW_IF_ERROR(sam_push_stack(sam_s0, sam_ssize, &sam_sp, v));  \
    } while (0)

#define POP_RETURN(ptr)                                                 \
    THROW_IF_ERROR(sam_pop_stack(sam_s0, sam_ssize, &sam_sp, ptr))
#define PUSH_RETURN(v)                                                  \
    do {                                                                \
        THROW_IF_ERROR(sam_push_stack(sam_s0, sam_ssize, &sam_sp, v));  \
    } while (0)


// Memory access

// Align a VM address
#define ALIGN(a)                                                \
    (((sam_uword_t)(a) + SAM_WORD_BYTES - 1) & (-SAM_WORD_BYTES))

// Check whether a VM address is aligned
#define IS_ALIGNED(a)                                           \
    (((uint8_t *)((sam_uword_t)(a) & (SAM_WORD_BYTES - 1)) == 0))

// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (SAM_UWORD_MAX >> (p))) << (p))

// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#if HAVE_ARITHMETIC_RSHIFT
#define ARSHIFT(n, p)                           \
    ((sam_word_t)(n) >> (p))
#else
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (sam_word_t)(LSHIFT(-((n) < 0), (SAM_WORD_BIT - p))))
#endif

// Traps
sam_word_t trap(sam_word_t code);
sam_word_t trap_libc(sam_uword_t function);

// GDB remote protocol stub
int gdb_init(int gdb_fdin, int gdb_fdout);
int handle_exception (int error);
