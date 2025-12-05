// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#cgo pkg-config: sdl2 SDL2_gfx
//#include "sam.h"
//#include "sam_opcodes.h"
import "C"
import "fmt"

type Word = C.sam_word_t
type Uword = C.sam_uword_t

var TAG_SHIFT = C.SAM_TAG_SHIFT
var TAG_MASK = C.SAM_TAG_MASK
var ATOM_TYPE_SHIFT = C.SAM_ATOM_TYPE_SHIFT
var ATOM_TYPE_MASK = C.SAM_ATOM_TYPE_MASK
var ARRAY_TYPE_SHIFT = C.SAM_ARRAY_TYPE_SHIFT
var ARRAY_TYPE_MASK = C.SAM_ARRAY_TYPE_MASK
var OPERAND_SHIFT = C.SAM_OPERAND_SHIFT
var OPERAND_MASK = C.SAM_OPERAND_MASK
var LINK_SHIFT = C.SAM_LINK_SHIFT
var LINK_MASK = C.SAM_LINK_MASK
var WORD_MASK = Uword(((1 << C.SAM_WORD_BIT) - 1))

type Stack struct {
	stack *C.sam_stack_t
}

var SamStack Stack

func NewStack() Stack {
	return Stack{stack: C.sam_stack_new()}
}

func (s *Stack) Sp() Uword {
	return s.stack.sp
}

func (s *Stack) StackPeek(addr Uword) (int, Uword) {
	var val Uword
	res := C.sam_stack_peek(s.stack, addr, &val)
	return int(res), val
}

func (s *Stack) StackPoke(addr Uword, val Uword) int {
	return int(C.sam_stack_poke(s.stack, addr, val))
}

func (s *Stack) PushStack(val Word) int {
	return int(C.sam_push_stack(s.stack, val))
}

func (s *Stack) PushLink(addr Uword) int {
	return int(C.sam_push_link(s.stack, addr))
}

func (s *Stack) PushAtom(atomType Uword, operand Uword) int {
	return int(C.sam_push_atom(s.stack, atomType, operand))
}

func (s *Stack) PushFloat(f float32) int {
	return int(C.sam_push_float(s.stack, C.sam_float_t(f)))
}

func (s *Stack) PushCode(stack Stack) int {
	return int(C.sam_push_code(s.stack, stack.stack.s0, stack.stack.sp))
}

func Run(pc0 Uword, pc Uword) Word {
	C.sam_pc0 = pc0
	C.sam_pc = pc
	return C.sam_run()
}

func Init() int {
	res := int(C.sam_init())
	SamStack = Stack{stack: C.sam_stack}
	return res
}

func DebugInit() int {
	return int(C.sam_debug_init())
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
	ERROR_HALT             = C.SAM_ERROR_HALT
	ERROR_INVALID_OPCODE   = C.SAM_ERROR_INVALID_OPCODE
	ERROR_INVALID_ADDRESS  = C.SAM_ERROR_INVALID_ADDRESS
	ERROR_STACK_UNDERFLOW  = C.SAM_ERROR_STACK_UNDERFLOW
	ERROR_STACK_OVERFLOW   = C.SAM_ERROR_STACK_OVERFLOW
	ERROR_WRONG_TYPE       = C.SAM_ERROR_WRONG_TYPE
	ERROR_BAD_BRACKET      = C.SAM_ERROR_BAD_BRACKET
	ERROR_INVALID_FUNCTION = C.SAM_ERROR_INVALID_FUNCTION
	ERROR_TRAP_INIT        = C.SAM_ERROR_TRAP_INIT
)

const (
	TAG_LINK  = C.SAM_TAG_LINK
	TAG_ATOM  = C.SAM_TAG_ATOM
	TAG_ARRAY = C.SAM_TAG_ARRAY
)

const (
	ATOM_INST  = C.SAM_ATOM_INST
	ATOM_INT   = C.SAM_ATOM_INT
	ATOM_FLOAT = C.SAM_ATOM_FLOAT
)

const (
	ARRAY_STACK = C.SAM_ARRAY_STACK
	ARRAY_RAW   = C.SAM_ARRAY_RAW
)

var errors = map[int]string{
	ERROR_OK:               "ERROR_OK",
	ERROR_HALT:             "ERROR_HALT",
	ERROR_INVALID_OPCODE:   "ERROR_INVALID_OPCODE",
	ERROR_INVALID_ADDRESS:  "ERROR_INVALID_ADDRESS",
	ERROR_STACK_UNDERFLOW:  "ERROR_STACK_UNDERFLOW",
	ERROR_STACK_OVERFLOW:   "ERROR_STACK_OVERFLOW",
	ERROR_WRONG_TYPE:       "ERROR_WRONG_TYPE",
	ERROR_BAD_BRACKET:      "ERROR_BAD_BRACKET",
	ERROR_INVALID_FUNCTION: "ERROR_INVALID_FUNCTION",
	ERROR_TRAP_INIT:        "ERROR_TRAP_INIT",
}

func ErrorMessage(code Word) string {
	baseCode := code & ^OPERAND_MASK
	if baseCode >= ERROR_OK && baseCode <= ERROR_TRAP_INIT {
		reasonCode := code >> OPERAND_SHIFT
		if baseCode == ERROR_HALT {
			return fmt.Sprintf("halt with reason %d (0x%x)", reasonCode, uint(reasonCode))
		}
		return fmt.Sprintf(errors[int(baseCode)])
	}
	return fmt.Sprintf("unknown code %d (0x%x)", code, uint(Uword(code)&WORD_MASK))
}

var Instructions = map[string]int{
	// Tag instructions
	"link": C.SAM_TAG_LINK,

	// Atom instructions
	"int":   C.SAM_TAG_ATOM | (C.SAM_ATOM_INT << C.SAM_ATOM_TYPE_SHIFT),
	"float": C.SAM_TAG_ATOM | (C.SAM_ATOM_FLOAT << C.SAM_ATOM_TYPE_SHIFT),

	// Niladic instructions
	"nop":   C.INST_NOP,
	"i2f":   C.INST_I2F,
	"f2i":   C.INST_F2I,
	"pop":   C.INST_POP,
	"get":   C.INST_GET,
	"set":   C.INST_SET,
	"iget":  C.INST_IGET,
	"iset":  C.INST_ISET,
	"do":    C.INST_DO,
	"if":    C.INST_IF,
	"while": C.INST_WHILE,
	"loop":  C.INST_LOOP,
	"not":   C.INST_NOT,
	"and":   C.INST_AND,
	"or":    C.INST_OR,
	"xor":   C.INST_XOR,
	"lsh":   C.INST_LSH,
	"rsh":   C.INST_RSH,
	"arsh":  C.INST_ARSH,
	"eq":    C.INST_EQ,
	"lt":    C.INST_LT,
	"neg":   C.INST_NEG,
	"add":   C.INST_ADD,
	"mul":   C.INST_MUL,
	"div":   C.INST_DIV,
	"rem":   C.INST_REM,
	"pow":   C.INST_POW,
	"sin":   C.INST_SIN,
	"cos":   C.INST_COS,
	"deg":   C.INST_DEG,
	"rad":   C.INST_RAD,
	"halt":  C.INST_HALT,

	// Trap
	"trap": C.SAM_TAG_ATOM | (C.SAM_ATOM_INST << C.SAM_ATOM_TYPE_SHIFT),
}

var Traps = map[string]int{
	"BLACK":          C.INST_BLACK,
	"WHITE":          C.INST_WHITE,
	"DISPLAY_WIDTH":  C.INST_DISPLAY_WIDTH,
	"DISPLAY_HEIGHT": C.INST_DISPLAY_HEIGHT,
	"CLEARSCREEN":    C.INST_CLEARSCREEN,
	"SETDOT":         C.INST_SETDOT,
	"DRAWLINE":       C.INST_DRAWLINE,
	"DRAWRECT":       C.INST_DRAWRECT,
	"DRAWROUNDRECT":  C.INST_DRAWROUNDRECT,
	"FILLRECT":       C.INST_FILLRECT,
	"DRAWCIRCLE":     C.INST_DRAWCIRCLE,
	"FILLCIRCLE":     C.INST_FILLCIRCLE,
	"DRAWBITMAP":     C.INST_DRAWBITMAP,
}
