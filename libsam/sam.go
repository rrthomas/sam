// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lSDL2 -lSDL2_gfx -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#include "sam.h"
//#include "sam_opcodes.h"
//#include "sam_traps.h"
import "C"

type Word = C.sam_word_t
type Uword = C.sam_uword_t

var OP_SHIFT = C.SAM_OP_SHIFT
var OP_MASK = C.SAM_OP_MASK

func Run() Word {
	return C.sam_run()
}

func Init(m0 []Word, msize Uword, sp Uword) Word {
	return C.sam_init(&m0[0], msize, sp)
}

func TrapsInit() Word {
	return C.sam_traps_init()
}

func TrapsFinish() {
	C.sam_traps_finish()
}

func TrapsWindowUsed() bool {
	return C.sam_traps_window_used() != 0
}

func TrapsWait() {
	C.sam_traps_wait()
}

func SetDebug(flag bool) {
	if flag {
		C.do_debug = true
	} else {
		C.do_debug = false
	}
}

func PrintStack() {
	C.sam_print_stack()
}

func DumpScreen(file string) {
	C.sam_dump_screen(C.CString(file))
}

const (
	ERROR_OK               = C.SAM_ERROR_OK
	ERROR_INVALID_OPCODE   = C.SAM_ERROR_INVALID_OPCODE
	ERROR_INVALID_ADDRESS  = C.SAM_ERROR_INVALID_ADDRESS
	ERROR_STACK_UNDERFLOW  = C.SAM_ERROR_STACK_UNDERFLOW
	ERROR_STACK_OVERFLOW   = C.SAM_ERROR_STACK_OVERFLOW
	ERROR_NOT_NUMBER       = C.SAM_ERROR_NOT_NUMBER
	ERROR_NOT_INT          = C.SAM_ERROR_NOT_INT
	ERROR_NOT_FLOAT        = C.SAM_ERROR_NOT_FLOAT
	ERROR_NOT_CODE         = C.SAM_ERROR_NOT_CODE
	ERROR_BAD_BRACKET      = C.SAM_ERROR_BAD_BRACKET
	ERROR_UNPAIRED_FLOAT   = C.SAM_ERROR_UNPAIRED_FLOAT
	ERROR_UNPAIRED_PUSH    = C.SAM_ERROR_UNPAIRED_PUSH
	ERROR_INVALID_SWAP     = C.SAM_ERROR_INVALID_SWAP
	ERROR_INVALID_FUNCTION = C.SAM_ERROR_INVALID_FUNCTION
	ERROR_BREAK            = C.SAM_ERROR_BREAK
	ERROR_TRAP_INIT        = C.SAM_ERROR_TRAP_INIT
)

const (
	INSN_NOP    = C.SAM_INSN_NOP
	INSN_INT    = C.SAM_INSN_INT
	INSN_FLOAT  = C.SAM_INSN_FLOAT
	INSN__FLOAT = C.SAM_INSN__FLOAT
	INSN_I2F    = C.SAM_INSN_I2F
	INSN_F2I    = C.SAM_INSN_F2I
	INSN_PUSH   = C.SAM_INSN_PUSH
	INSN__PUSH  = C.SAM_INSN__PUSH
	INSN_POP    = C.SAM_INSN_POP
	INSN_DUP    = C.SAM_INSN_DUP
	INSN_SWAP   = C.SAM_INSN_SWAP
	INSN_IDUP   = C.SAM_INSN_IDUP
	INSN_ISET   = C.SAM_INSN_ISET
	INSN_BRA    = C.SAM_INSN_BRA
	INSN_KET    = C.SAM_INSN_KET
	INSN_LINK   = C.SAM_INSN_LINK
	INSN_DO     = C.SAM_INSN_DO
	INSN_IF     = C.SAM_INSN_IF
	INSN_WHILE  = C.SAM_INSN_WHILE
	INSN_LOOP   = C.SAM_INSN_LOOP
	INSN_NOT    = C.SAM_INSN_NOT
	INSN_AND    = C.SAM_INSN_AND
	INSN_OR     = C.SAM_INSN_OR
	INSN_XOR    = C.SAM_INSN_XOR
	INSN_LSH    = C.SAM_INSN_LSH
	INSN_RSH    = C.SAM_INSN_RSH
	INSN_ARSH   = C.SAM_INSN_ARSH
	INSN_EQ     = C.SAM_INSN_EQ
	INSN_LT     = C.SAM_INSN_LT
	INSN_NEG    = C.SAM_INSN_NEG
	INSN_ADD    = C.SAM_INSN_ADD
	INSN_MUL    = C.SAM_INSN_MUL
	INSN_DIV    = C.SAM_INSN_DIV
	INSN_REM    = C.SAM_INSN_REM
	INSN_POW    = C.SAM_INSN_POW
	INSN_SIN    = C.SAM_INSN_SIN
	INSN_COS    = C.SAM_INSN_COS
	INSN_DEG    = C.SAM_INSN_DEG
	INSN_RAD    = C.SAM_INSN_RAD
	INSN_HALT   = C.SAM_INSN_HALT
	INSN_TRAP   = C.SAM_INSN_TRAP
)

var Instructions = map[string]int{
	"nop":    INSN_NOP,
	"int":    INSN_INT,
	"float":  INSN_FLOAT,
	"_float": INSN__FLOAT,
	"i2f":    INSN_I2F,
	"f2i":    INSN_F2I,
	"push":   INSN_PUSH,
	"_push":  INSN__PUSH,
	"pop":    INSN_POP,
	"dup":    INSN_DUP,
	"swap":   INSN_SWAP,
	"idup":   INSN_IDUP,
	"iset":   INSN_ISET,
	"bra":    INSN_BRA,
	"ket":    INSN_KET,
	"link":   INSN_LINK,
	"do":     INSN_DO,
	"if":     INSN_IF,
	"while":  INSN_WHILE,
	"loop":   INSN_LOOP,
	"not":    INSN_NOT,
	"and":    INSN_AND,
	"or":     INSN_OR,
	"xor":    INSN_XOR,
	"lsh":    INSN_LSH,
	"rsh":    INSN_RSH,
	"arsh":   INSN_ARSH,
	"eq":     INSN_EQ,
	"lt":     INSN_LT,
	"neg":    INSN_NEG,
	"add":    INSN_ADD,
	"mul":    INSN_MUL,
	"div":    INSN_DIV,
	"rem":    INSN_REM,
	"pow":    INSN_POW,
	"sin":    INSN_SIN,
	"cos":    INSN_COS,
	"deg":    INSN_DEG,
	"rad":    INSN_RAD,
	"halt":   INSN_HALT,
	"trap":   INSN_TRAP,
}

var Traps = map[string]int{
	"BLACK":          C.TRAP_BLACK,
	"WHITE":          C.TRAP_WHITE,
	"DISPLAY_WIDTH":  C.TRAP_DISPLAY_WIDTH,
	"DISPLAY_HEIGHT": C.TRAP_DISPLAY_HEIGHT,
	"CLEARSCREEN":    C.TRAP_CLEARSCREEN,
	"SETDOT":         C.TRAP_SETDOT,
	"DRAWLINE":       C.TRAP_DRAWLINE,
	"DRAWRECT":       C.TRAP_DRAWRECT,
	"DRAWROUNDRECT":  C.TRAP_DRAWROUNDRECT,
	"FILLRECT":       C.TRAP_FILLRECT,
	"INVERTRECT":     C.TRAP_INVERTRECT,
	"DRAWCIRCLE":     C.TRAP_DRAWCIRCLE,
	"FILLCIRCLE":     C.TRAP_FILLCIRCLE,
	"DRAWBITMAP":     C.TRAP_DRAWBITMAP,
}
