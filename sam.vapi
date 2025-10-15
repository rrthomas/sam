// VAPI for libsam.

[CCode(cheader_filename = "sam.h", cprefix = "SAM_", lower_case_cprefix = "sam_")]
namespace Sam {

// Basic types
    [SimpleType]
    [IntegerType(rank = 6)]
    public struct word_t {}

    [SimpleType]
    [IntegerType(rank = 7)]
    public struct uword_t {}

    [SimpleType]
    [FloatingType(rank = 1)]
    public struct float_t {}

    public const int32 WORD_BYTES;
    public const int32 WORD_BIT;
    public const int32 WORD_MIN;
    public const int32 INT_MIN;
    public const uint32 UWORD_MAX;

    //// VM registers
    // #define R(reg, type)                     \
    //         extern type sam_##reg;
    // #include "sam_registers.h"
    // #undef R

    // Error codes
    [CCode(cprefix = "ERROR_", has_type_id = false)]
    public enum Error {
        OK,
        INVALID_OPCODE,
        INVALID_ADDRESS,
        STACK_UNDERFLOW,
        STACK_OVERFLOW,
        NOT_NUMBER,
        NOT_INT,
        NOT_FLOAT,
        NOT_CODE,
        BAD_BRACKET,
        UNPAIRED_FLOAT,
        UNPAIRED_PUSH,
        INVALID_SWAP,
        INVALID_FUNCTION,
        BREAK,
        TRAP_INIT,
    }

    //// Stack access
    // int sam_stack_peek(sam_uword_t addr, sam_uword_t* val);
    // int sam_stack_poke(sam_uword_t addr, sam_uword_t val);
    // sam_word_t sam_stack_swap(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2);
    // int sam_stack_item(sam_word_t n, sam_uword_t* addr, sam_uword_t* size);
    // int sam_find_code(sam_uword_t code, sam_uword_t* addr);
    // int sam_pop_stack(sam_word_t* val_ptr);
    // int sam_push_stack(sam_word_t val);

    // Miscellaneous routines
    public word_t run();
    public int init(word_t* m0, uword_t msize, uword_t sp);

    //// Portable left shift (the behaviour of << with overflow (including on any
    //// negative number) is undefined)
    // #define LSHIFT(n, p)                         \
    //         (((n) & (SAM_UWORD_MAX >> (p))) << (p))

    // Debug
    public bool do_debug;
    public void print_stack();
    public void print_working_stack();

    [PrintfFunction]
    public void debug(string fmt, ...);

    //// Traps
    public const uint DISPLAY_WIDTH;
    public const uint DISPLAY_HEIGHT;
    public const uint update_interval;

    public word_t traps_init();
    public void traps_finish();

    public int traps_process_events();

    // sam_word_t trap(uword_t function);
    bool traps_window_used();

    // uint32_t getpixel(int x, int y);
    void dump_screen(string filename);

    // void update_screen();
}
