// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#cgo pkg-config: sdl2 SDL2_gfx
//#include "sam.h"
//#include "sam_opcodes.h"
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

func StackPeek(addr Uword) (int, Uword) {
	var val Uword
	res := C.sam_stack_peek(addr, &val)
	return int(res), val
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
	ERROR_INVALID_FUNCTION = C.SAM_ERROR_INVALID_FUNCTION
	ERROR_TRAP_INIT        = C.SAM_ERROR_TRAP_INIT
)

const (
	INSN_TRAP   = C.SAM_INSN_TRAP
	INSN_INT    = C.SAM_INSN_INT
	INSN_FLOAT  = C.SAM_INSN_FLOAT
	INSN__FLOAT = C.SAM_INSN__FLOAT
	INSN_PUSH   = C.SAM_INSN_PUSH
	INSN__PUSH  = C.SAM_INSN__PUSH
	INSN_STACK  = C.SAM_INSN_STACK
	INSN_LINK   = C.SAM_INSN_LINK
)

var Instructions = map[string]int{
	"trap":   INSN_TRAP,
	"int":    INSN_INT,
	"float":  INSN_FLOAT,
	"_float": INSN__FLOAT,
	"push":   INSN_PUSH,
	"_push":  INSN__PUSH,
	"stack":  INSN_STACK,
	"link":   INSN_LINK,

	// Instructions encoded as traps.
	"nop":   INSN_TRAP | (C.TRAP_NOP << OP_SHIFT),
	"i2f":   INSN_TRAP | (C.TRAP_I2F << OP_SHIFT),
	"f2i":   INSN_TRAP | (C.TRAP_F2I << OP_SHIFT),
	"pop":   INSN_TRAP | (C.TRAP_POP << OP_SHIFT),
	"get":   INSN_TRAP | (C.TRAP_GET << OP_SHIFT),
	"set":   INSN_TRAP | (C.TRAP_SET << OP_SHIFT),
	"iget":  INSN_TRAP | (C.TRAP_IGET << OP_SHIFT),
	"iset":  INSN_TRAP | (C.TRAP_ISET << OP_SHIFT),
	"do":    INSN_TRAP | (C.TRAP_DO << OP_SHIFT),
	"if":    INSN_TRAP | (C.TRAP_IF << OP_SHIFT),
	"while": INSN_TRAP | (C.TRAP_WHILE << OP_SHIFT),
	"loop":  INSN_TRAP | (C.TRAP_LOOP << OP_SHIFT),
	"not":   INSN_TRAP | (C.TRAP_NOT << OP_SHIFT),
	"and":   INSN_TRAP | (C.TRAP_AND << OP_SHIFT),
	"or":    INSN_TRAP | (C.TRAP_OR << OP_SHIFT),
	"xor":   INSN_TRAP | (C.TRAP_XOR << OP_SHIFT),
	"lsh":   INSN_TRAP | (C.TRAP_LSH << OP_SHIFT),
	"rsh":   INSN_TRAP | (C.TRAP_RSH << OP_SHIFT),
	"arsh":  INSN_TRAP | (C.TRAP_ARSH << OP_SHIFT),
	"eq":    INSN_TRAP | (C.TRAP_EQ << OP_SHIFT),
	"lt":    INSN_TRAP | (C.TRAP_LT << OP_SHIFT),
	"neg":   INSN_TRAP | (C.TRAP_NEG << OP_SHIFT),
	"add":   INSN_TRAP | (C.TRAP_ADD << OP_SHIFT),
	"mul":   INSN_TRAP | (C.TRAP_MUL << OP_SHIFT),
	"div":   INSN_TRAP | (C.TRAP_DIV << OP_SHIFT),
	"rem":   INSN_TRAP | (C.TRAP_REM << OP_SHIFT),
	"pow":   INSN_TRAP | (C.TRAP_POW << OP_SHIFT),
	"sin":   INSN_TRAP | (C.TRAP_SIN << OP_SHIFT),
	"cos":   INSN_TRAP | (C.TRAP_COS << OP_SHIFT),
	"deg":   INSN_TRAP | (C.TRAP_DEG << OP_SHIFT),
	"rad":   INSN_TRAP | (C.TRAP_RAD << OP_SHIFT),
	"halt":  INSN_TRAP | (C.TRAP_HALT << OP_SHIFT),
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
	"DRAWCIRCLE":     C.TRAP_DRAWCIRCLE,
	"FILLCIRCLE":     C.TRAP_FILLCIRCLE,
	"DRAWBITMAP":     C.TRAP_DRAWBITMAP,
}
